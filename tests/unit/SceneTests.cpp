#include "engine/core/AnimationSystem.h"
#include "engine/core/PhysicsSystem.h"
#include "engine/core/Scene.h"
#include "engine/core/SystemRegistry.h"
#include "engine/core/SystemScheduler.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/RenderLayer.h"
#include "engine/core/components/Transform.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <future>
#include <memory>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace
{
static_assert(std::is_same_v<decltype(std::declval<Scene &>().getGameObjects()), std::vector<GameObject> &>);
static_assert(
    std::is_same_v<decltype(std::declval<const Scene &>().getGameObjects()), const std::vector<GameObject> &>);
static_assert(std::is_same_v<decltype(std::declval<Scene &>().findGameObjectByUuid(0)), GameObject *>);
static_assert(std::is_same_v<decltype(std::declval<const Scene &>().findGameObjectByUuid(0)), const GameObject *>);
static_assert(std::is_same_v<decltype(std::declval<Scene &>().findPrimaryCamera()), GameObject *>);
static_assert(std::is_same_v<decltype(std::declval<const Scene &>().findPrimaryCamera()), const GameObject *>);

class CountingSystem : public ISystem
{
public:
    void update(SystemContext &) override
    {
        ++updateCount;
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.CountingSystem")
            .onMainThread()
            .nonReentrant();
    }

    static int updateCount;
};

int CountingSystem::updateCount = 0;

std::vector<std::string> graphUpdates;

struct GraphState
{
};

class RecordingGraphSystem : public ISystem
{
public:
    void update(SystemContext &) override
    {
        graphUpdates.push_back(descriptor().name());
    }
};

class GraphHeadSystem final : public RecordingGraphSystem
{
public:
    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.GraphHead")
            .writes<GraphState>()
            .before("SceneTests.GraphMiddle");
    }
};

class GraphMiddleSystem final : public RecordingGraphSystem
{
public:
    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.GraphMiddle")
            .writes<GraphState>()
            .after("SceneTests.GraphHead")
            .before("SceneTests.GraphTail");
    }
};

class GraphTailSystem final : public RecordingGraphSystem
{
public:
    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.GraphTail").writes<GraphState>();
    }
};

class GraphCycleFirstSystem final : public RecordingGraphSystem
{
public:
    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.GraphCycleFirst").before("SceneTests.GraphCycleSecond");
    }
};

class GraphCycleSecondSystem final : public RecordingGraphSystem
{
public:
    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.GraphCycleSecond").before("SceneTests.GraphCycleFirst");
    }
};

void registerGraphSystems()
{
    auto &systems = SystemRegistry::instance();
    systems.registerSystem<GraphHeadSystem>("SceneTests.GraphHead");
    systems.registerSystem<GraphMiddleSystem>("SceneTests.GraphMiddle");
    systems.registerSystem<GraphTailSystem>("SceneTests.GraphTail");
    systems.registerSystem<GraphCycleFirstSystem>("SceneTests.GraphCycleFirst");
    systems.registerSystem<GraphCycleSecondSystem>("SceneTests.GraphCycleSecond");
    graphUpdates.clear();
}

void addGraphChain(Scene &scene)
{
    scene.addSystem("SceneTests.GraphTail");
    scene.addSystem("SceneTests.GraphMiddle");
    scene.addSystem("SceneTests.GraphHead");
    scene.setAllSystemsEnabled(true);
}

YAML::Node serializeToNode(Scene &scene)
{
    YAML::Emitter out;
    scene.serialize(out);
    EXPECT_TRUE(out.good()) << "YAML emitter failed: " << out.GetLastError();
    return YAML::Load(out.c_str());
}

struct ConcurrentSceneState
{
    entt::entity entity = entt::null;
    bool fail = false;
    std::promise<void> destroyed;
    std::promise<void> peerStarted;
    std::shared_future<void> releasePeer;
    std::atomic<bool> peerFinished{false};
    int consumerCalls = 0;
    float observedDelta = 0.0f;
};

std::shared_ptr<ConcurrentSceneState> concurrentSceneState;

class ConcurrentSceneDestroyer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        const auto state = concurrentSceneState;
        context.entities.destroy(state->entity);
        state->destroyed.set_value();
        if (state->fail)
        {
            throw std::runtime_error("concurrent scene update failed");
        }
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.ConcurrentDestroyer").requestsStructuralChanges();
    }
};

class ConcurrentScenePeer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        using namespace std::chrono_literals;
        const auto state = concurrentSceneState;
        state->peerStarted.set_value();
        if (state->releasePeer.wait_for(5s) != std::future_status::ready)
        {
            throw std::runtime_error("test did not release scene peer");
        }
        state->observedDelta = context.frame.deltaTime;
        state->peerFinished.store(true, std::memory_order_relaxed);
    }

    SystemDescriptor descriptor() const override
    {
        // This peer only reads the immutable frame, so it can overlap deletion.
        return SystemDescriptor::named("SceneTests.ConcurrentPeer");
    }
};

class ConcurrentSceneConsumer final : public ISystem
{
public:
    void update(SystemContext &) override
    {
        ++concurrentSceneState->consumerCalls;
        EXPECT_TRUE(concurrentSceneState->peerFinished.load(std::memory_order_relaxed));
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.ConcurrentConsumer")
            .after("SceneTests.ConcurrentDestroyer")
            .after("SceneTests.ConcurrentPeer")
            .onMainThread();
    }
};

struct EnableSnapshotState
{
    Scene &scene;
    int changes = 0;
    int updates = 0;
};

std::shared_ptr<EnableSnapshotState> enableSnapshotState;

class SnapshotEnableChanger final : public ISystem
{
public:
    void update(SystemContext &) override
    {
        auto &state = *enableSnapshotState;
        ++state.changes;
        EXPECT_TRUE(state.scene.setSystemEnabled("SceneTests.SnapshotConsumer", state.changes == 1));
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.SnapshotChanger")
            .before("SceneTests.SnapshotConsumer")
            .onMainThread();
    }
};

class SnapshotEnabledConsumer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        auto view = context.entities.view<const GraphState>();
        EXPECT_EQ(view.begin(), view.end());
        ++enableSnapshotState->updates;
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SceneTests.SnapshotConsumer").reads<GraphState>();
    }
};
} // namespace

TEST(SceneTests, EnableChangesDuringAnUpdateApplyToTheNextTick)
{
    auto &systems = SystemRegistry::instance();
    systems.registerSystem<SnapshotEnableChanger>("SceneTests.SnapshotChanger");
    systems.registerSystem<SnapshotEnabledConsumer>("SceneTests.SnapshotConsumer");
    for (const std::size_t workerCount : {0U, 2U})
    {
        SCOPED_TRACE(workerCount);
        SystemScheduler scheduler{JobSystemConfig{workerCount, 1, false}};
        Scene scene{"enable snapshot scene"};
        enableSnapshotState = std::make_shared<EnableSnapshotState>(EnableSnapshotState{scene});
        scene.addSystem("SceneTests.SnapshotChanger");
        scene.addSystem("SceneTests.SnapshotConsumer");
        ASSERT_TRUE(scene.setSystemEnabled("SceneTests.SnapshotChanger", true));
        scene.play();

        scene.update(FrameContext{}, scheduler);
        EXPECT_EQ(enableSnapshotState->updates, 0);
        scene.update(FrameContext{}, scheduler);
        EXPECT_EQ(enableSnapshotState->updates, 1);
        scene.update(FrameContext{}, scheduler);
        EXPECT_EQ(enableSnapshotState->updates, 1);
    }
}

TEST(SceneTests, ConcurrentRuntimeTickWaitsForItsBatchBeforeCleanupOrFailureReturns)
{
    using namespace std::chrono_literals;
    auto &systems = SystemRegistry::instance();
    systems.registerSystem<ConcurrentSceneDestroyer>("SceneTests.ConcurrentDestroyer");
    systems.registerSystem<ConcurrentScenePeer>("SceneTests.ConcurrentPeer");
    systems.registerSystem<ConcurrentSceneConsumer>("SceneTests.ConcurrentConsumer");
    SystemScheduler scheduler{JobSystemConfig{2, 1, false}};

    for (const bool fail : {false, true})
    {
        SCOPED_TRACE(fail);
        concurrentSceneState = std::make_shared<ConcurrentSceneState>();
        const auto state = concurrentSceneState;
        state->fail = fail;
        auto destroyed = state->destroyed.get_future();
        auto peerStarted = state->peerStarted.get_future();
        std::promise<void> releasePeer;
        state->releasePeer = releasePeer.get_future().share();
        Scene scene{"concurrent runtime scene"};
        state->entity = scene.addGameObject(8, 8, "removed during batch").getEntityId();
        scene.addSystem("SceneTests.ConcurrentConsumer");
        scene.addSystem("SceneTests.ConcurrentDestroyer");
        scene.addSystem("SceneTests.ConcurrentPeer");
        scene.setAllSystemsEnabled(true);
        scene.play();
        FrameContext frame;
        frame.deltaTime = 1.0f / 60.0f;
        auto tick = std::async(std::launch::async, [&]
            {
                scene.update(frame, scheduler);
            });

        EXPECT_EQ(destroyed.wait_for(2s), std::future_status::ready);
        EXPECT_EQ(peerStarted.wait_for(2s), std::future_status::ready);
        EXPECT_EQ(tick.wait_for(20ms), std::future_status::timeout);
        releasePeer.set_value();
        if (fail)
        {
            EXPECT_THROW(tick.get(), std::runtime_error);
        }
        else
        {
            EXPECT_NO_THROW(tick.get());
        }

        EXPECT_TRUE(state->peerFinished.load(std::memory_order_relaxed));
        EXPECT_FLOAT_EQ(state->observedDelta, frame.deltaTime);
        EXPECT_EQ(state->consumerCalls, fail ? 0 : 1);
        EXPECT_TRUE(scene.getGameObjects().empty());
    }
}

TEST(SceneTests, ConstQueriesReturnReadOnlyObjectsAndPreserveMissingResults)
{
    Scene scene{std::string{"const_queries"}};
    const Scene &readOnly = scene;
    EXPECT_TRUE(readOnly.getGameObjects().empty());
    EXPECT_EQ(readOnly.findGameObjectByUuid(42), nullptr);
    EXPECT_EQ(readOnly.findPrimaryCamera(), nullptr);

    GameObject &camera = scene.addGameObject(32, 48, std::string{"camera"});
    const auto uuid = camera.getComponent<EntityInfo>().uuid;
    camera.addComponent<Camera>();
    camera.getComponent<Camera>().primary = false;
    EXPECT_EQ(readOnly.findPrimaryCamera(), nullptr);

    camera.getComponent<Camera>().primary = true;
    ASSERT_EQ(readOnly.getGameObjects().size(), 1U);
    EXPECT_EQ(readOnly.findGameObjectByUuid(uuid), &camera);
    EXPECT_EQ(readOnly.findPrimaryCamera(), &camera);
    EXPECT_EQ(&readOnly.getGameObjects().front(), &camera);
    EXPECT_EQ(readOnly.findGameObjectByUuid(uuid), scene.findGameObjectByUuid(uuid));
    EXPECT_EQ(readOnly.findPrimaryCamera(), scene.findPrimaryCamera());
}

TEST(SceneTests, DisabledSystemDoesNotUpdateAndEnablementIsNotSerialized)
{
    constexpr const char *systemName = "SceneTests.CountingSystem";
    SystemRegistry::instance().registerSystem<CountingSystem>(systemName);
    CountingSystem::updateCount = 0;

    Scene scene{"system test"};
    scene.addSystem(systemName);
    scene.setAllSystemsEnabled(true);
    scene.play();
    scene.update(FrameContext{});
    EXPECT_EQ(CountingSystem::updateCount, 1);

    ASSERT_TRUE(scene.setSystemEnabled(systemName, false));
    scene.update(FrameContext{});
    EXPECT_EQ(CountingSystem::updateCount, 1);

    const auto info = scene.getSystemInfo();
    const auto systemInfo = std::find_if(info.begin(),
        info.end(),
        [&](const Scene::SystemInfo &entry)
        {
            return entry.name == systemName;
        });
    ASSERT_NE(systemInfo, info.end());
    EXPECT_FALSE(systemInfo->enabled);

    ASSERT_TRUE(scene.setSystemEnabled(systemName, true));
    Scene clone = scene.clone("system test clone");
    const auto clonedInfo = clone.getSystemInfo();
    const auto clonedSystemInfo = std::find_if(clonedInfo.begin(),
        clonedInfo.end(),
        [&](const Scene::SystemInfo &entry)
        {
            return entry.name == systemName;
        });
    ASSERT_NE(clonedSystemInfo, clonedInfo.end());
    EXPECT_FALSE(clonedSystemInfo->enabled);
}

TEST(SceneTests, EngineSystemsUseTheGenericSystemInterface)
{
    SystemRegistry::instance().registerSystem<AnimationSystem>("AnimationSystem");
    SystemRegistry::instance().registerSystem<PhysicsSystem>("PhysicsSystem");

    Scene scene{"system test"};
    scene.addSystem("AnimationSystem");
    scene.addSystem("PhysicsSystem");
    const auto info = scene.getSystemInfo();

    const auto animation = std::find_if(info.begin(),
        info.end(),
        [](const Scene::SystemInfo &entry)
        {
            return entry.name == "AnimationSystem";
        });
    const auto physics = std::find_if(info.begin(),
        info.end(),
        [](const Scene::SystemInfo &entry)
        {
            return entry.name == "PhysicsSystem";
        });
    ASSERT_NE(animation, info.end());
    ASSERT_NE(physics, info.end());

    scene.setAllSystemsEnabled(false);
    const auto disabledInfo = scene.getSystemInfo();
    EXPECT_TRUE(std::all_of(disabledInfo.begin(),
        disabledInfo.end(),
        [](const Scene::SystemInfo &entry)
        {
            return !entry.enabled;
        }));
}

TEST(SceneTests, EnabledStateControlsEditModeExecution)
{
    constexpr const char *systemName = "SceneTests.EditModeSystem";
    SystemRegistry::instance().registerSystem<CountingSystem>(systemName);
    CountingSystem::updateCount = 0;

    Scene scene{"edit mode system test"};
    scene.addSystem(systemName);
    scene.update(FrameContext{}, true);
    EXPECT_EQ(CountingSystem::updateCount, 0);

    ASSERT_TRUE(scene.setSystemEnabled(systemName, true));
    scene.update(FrameContext{}, true);
    EXPECT_EQ(CountingSystem::updateCount, 1);

    ASSERT_TRUE(scene.setSystemEnabled(systemName, false));
    scene.update(FrameContext{}, true);
    EXPECT_EQ(CountingSystem::updateCount, 1);

    scene.pause();
    scene.update(FrameContext{});
    EXPECT_EQ(CountingSystem::updateCount, 1);
}

TEST(SceneTests, SystemRegistryCreatesIndependentSystemInstances)
{
    constexpr const char *systemName = "SceneTests.FactorySystem";
    SystemRegistry::instance().registerSystem<CountingSystem>(systemName);

    auto first = SystemRegistry::instance().create(systemName);
    auto second = SystemRegistry::instance().create(systemName);

    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_NE(first.get(), second.get());
}

TEST(SceneTests, DescriptorDependenciesOverrideRegistrationOrderAndAppearInSystemInfo)
{
    registerGraphSystems();
    Scene scene{"graph scene"};
    addGraphChain(scene);
    scene.play();
    scene.update(FrameContext{});

    const std::vector<std::string> expected{
        "SceneTests.GraphHead", "SceneTests.GraphMiddle", "SceneTests.GraphTail"};
    EXPECT_EQ(graphUpdates, expected);

    std::vector<std::string> displayedNames;
    for (const auto &info : scene.getSystemInfo())
    {
        displayedNames.push_back(info.name);
    }
    EXPECT_EQ(displayedNames, expected);
}

TEST(SceneTests, FactoryAliasesPreserveDescriptorDependenciesAndSerializedNames)
{
    registerGraphSystems();
    constexpr const char *headAlias = "SceneTests.HeadFactoryAlias";
    SystemRegistry::instance().registerSystem<GraphHeadSystem>(headAlias);

    Scene scene{"aliased graph scene"};
    scene.addSystem("SceneTests.GraphTail");
    scene.addSystem("SceneTests.GraphMiddle");
    scene.addSystem(headAlias);
    scene.setAllSystemsEnabled(true);
    scene.update(FrameContext{}, true);

    const std::vector<std::string> expectedUpdates{
        "SceneTests.GraphHead", "SceneTests.GraphMiddle", "SceneTests.GraphTail"};
    EXPECT_EQ(graphUpdates, expectedUpdates);
    const auto info = scene.getSystemInfo();
    ASSERT_EQ(info.size(), 3u);
    EXPECT_EQ(info.front().name, headAlias);

    graphUpdates.clear();
    ASSERT_TRUE(scene.setSystemEnabled(headAlias, false));
    scene.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, (std::vector<std::string>{"SceneTests.GraphMiddle", "SceneTests.GraphTail"}));

    const auto serializedSystems = serializeToNode(scene)["aliased graph scene"]["Systems"];
    ASSERT_EQ(serializedSystems.size(), 3u);
    EXPECT_EQ(serializedSystems[0].as<std::string>(), "SceneTests.GraphTail");
    EXPECT_EQ(serializedSystems[1].as<std::string>(), "SceneTests.GraphMiddle");
    EXPECT_EQ(serializedSystems[2].as<std::string>(), headAlias);

    Scene clone = scene.clone("aliased graph clone");
    const auto clonedInfo = clone.getSystemInfo();
    ASSERT_EQ(clonedInfo.size(), 3u);
    EXPECT_EQ(clonedInfo.front().name, headAlias);
    EXPECT_FALSE(clonedInfo.front().enabled);
    clone.setAllSystemsEnabled(true);
    Scene moved{std::move(clone)};
    graphUpdates.clear();
    moved.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, expectedUpdates);
}

TEST(SceneTests, AddingPreviouslyAbsentDependencyRebuildsTheExecutionOrder)
{
    registerGraphSystems();
    Scene scene{"changing graph scene"};
    scene.addSystem("SceneTests.GraphTail");
    scene.addSystem("SceneTests.GraphHead");
    scene.setAllSystemsEnabled(true);
    scene.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, (std::vector<std::string>{"SceneTests.GraphTail", "SceneTests.GraphHead"}));

    graphUpdates.clear();
    scene.addSystem("SceneTests.GraphMiddle");
    scene.setAllSystemsEnabled(true);
    scene.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, (std::vector<std::string>{
                               "SceneTests.GraphHead", "SceneTests.GraphMiddle", "SceneTests.GraphTail"}));
}

TEST(SceneTests, DisablingAnIntermediateSystemPreservesDependenciesAndCanBeReenabled)
{
    registerGraphSystems();
    Scene scene{"toggle graph scene"};
    addGraphChain(scene);
    ASSERT_TRUE(scene.setSystemEnabled("SceneTests.GraphMiddle", false));
    scene.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, (std::vector<std::string>{"SceneTests.GraphHead", "SceneTests.GraphTail"}));

    graphUpdates.clear();
    ASSERT_TRUE(scene.setSystemEnabled("SceneTests.GraphMiddle", true));
    scene.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, (std::vector<std::string>{
                               "SceneTests.GraphHead", "SceneTests.GraphMiddle", "SceneTests.GraphTail"}));
}

TEST(SceneTests, InvalidAdditionIsRejectedBeforeUpdatesAndLeavesThePreviousGraphUsable)
{
    registerGraphSystems();
    Scene scene{"invalid graph scene"};
    scene.addSystem("SceneTests.GraphCycleFirst");
    scene.setAllSystemsEnabled(true);

    EXPECT_THROW(scene.addSystem("SceneTests.GraphCycleSecond"), std::invalid_argument);
    EXPECT_TRUE(graphUpdates.empty());
    const auto info = scene.getSystemInfo();
    ASSERT_EQ(info.size(), 1u);
    EXPECT_EQ(info.front().name, "SceneTests.GraphCycleFirst");
    EXPECT_TRUE(info.front().enabled);

    scene.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, (std::vector<std::string>{"SceneTests.GraphCycleFirst"}));
}

TEST(SceneTests, DeserializationRejectsACycleBeforeAnySystemUpdates)
{
    registerGraphSystems();
    EXPECT_THROW((Scene{YAML::Load(R"(
invalid graph scene:
  Systems: [SceneTests.GraphCycleFirst, SceneTests.GraphCycleSecond]
  GameObjects: []
)")}), std::invalid_argument);
    EXPECT_TRUE(graphUpdates.empty());
}

TEST(SceneTests, ClonedAndMovedScenesRetainDescriptorExecutionOrder)
{
    registerGraphSystems();
    Scene original{"original graph scene"};
    addGraphChain(original);

    Scene clone = original.clone("cloned graph scene");
    clone.setAllSystemsEnabled(true);
    clone.update(FrameContext{}, true);
    const std::vector<std::string> expected{
        "SceneTests.GraphHead", "SceneTests.GraphMiddle", "SceneTests.GraphTail"};
    EXPECT_EQ(graphUpdates, expected);

    graphUpdates.clear();
    Scene moved{std::move(clone)};
    moved.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, expected);

    graphUpdates.clear();
    Scene assigned{"destination graph scene"};
    assigned.addSystem("SceneTests.GraphTail");
    assigned = std::move(moved);
    assigned.update(FrameContext{}, true);
    EXPECT_EQ(graphUpdates, expected);
}

TEST(SceneTests, SerializesGameObjectsAsTaggedEntries)
{
    Scene scene{"test scene"};
    auto &gameObject = scene.addGameObject(32, 48, "hero");
    gameObject.getComponent<Transform>().position = {12.0f, 34.0f, 0.0f};

    const auto root = serializeToNode(scene);
    ASSERT_TRUE(root["test scene"].IsDefined());

    const auto gameObjects = root["test scene"]["GameObjects"];
    ASSERT_TRUE(gameObjects.IsSequence());
    ASSERT_EQ(gameObjects.size(), 1);
    ASSERT_TRUE(gameObjects[0]["hero"].IsDefined());

    const auto components = gameObjects[0]["hero"];
    EXPECT_EQ(components["EntityInfo"]["Tag"].as<std::string>(), "hero");
    EXPECT_EQ(components["EntityInfo"]["Width"].as<unsigned int>(), 32);
    EXPECT_EQ(components["EntityInfo"]["Height"].as<unsigned int>(), 48);
    EXPECT_FLOAT_EQ(components["Transform"]["Position"][0].as<float>(), 12.0f);
}

TEST(SceneTests, DeserializesGameObjectsFromTaggedEntries)
{
    const auto root = YAML::Load(R"(
test scene:
  GameObjects:
    - hero:
        EntityInfo:
          Tag: hero
          Width: 32
          Height: 48
        Transform:
          Position: [12, 34, 0]
          Rotation: [0, 0, 0]
          Scale: [1, 1, 1]
        RenderLayer:
          Layer: World
          Order: 0
          Mask: 4294967295
  Systems: []
)");

    Scene scene{std::move(root)};

    ASSERT_EQ(scene.getGameObjects().size(), 1);
    const auto &gameObject = scene.getGameObjects()[0];
    EXPECT_EQ(gameObject.getComponent<EntityInfo>().tag, "hero");
    EXPECT_EQ(gameObject.getComponent<EntityInfo>().width, 32);
    EXPECT_EQ(gameObject.getComponent<EntityInfo>().height, 48);
    EXPECT_FLOAT_EQ(gameObject.getComponent<Transform>().position.x, 12.0f);
}

TEST(SceneTests, RoundTripsSerializedSceneThroughDeserializer)
{
    Scene original{"round trip scene"};

    auto &hero = original.addGameObject(32, 48, "hero");
    hero.getComponent<Transform>().position = {12.0f, 34.0f, 0.0f};
    hero.getComponent<Transform>().rotation = {0.0f, 0.0f, 15.0f};
    hero.getComponent<RenderLayer>().order = 2;

    auto &crate = original.addGameObject(16, 16, "crate");
    crate.getComponent<Transform>().position = {80.0f, 96.0f, 0.0f};
    crate.getComponent<RenderLayer>().order = 5;

    YAML::Emitter out;
    original.serialize(out);
    ASSERT_TRUE(out.good()) << "YAML emitter failed: " << out.GetLastError();

    Scene copy{YAML::Load(out.c_str())};

    ASSERT_EQ(copy.getGameObjects().size(), 2);

    const auto &copiedHero = copy.getGameObjects()[0];
    EXPECT_EQ(copiedHero.getComponent<EntityInfo>().tag, "hero");
    EXPECT_EQ(copiedHero.getComponent<EntityInfo>().width, 32);
    EXPECT_EQ(copiedHero.getComponent<EntityInfo>().height, 48);
    EXPECT_FLOAT_EQ(copiedHero.getComponent<Transform>().position.x, 12.0f);
    EXPECT_FLOAT_EQ(copiedHero.getComponent<Transform>().position.y, 34.0f);
    EXPECT_FLOAT_EQ(copiedHero.getComponent<Transform>().rotation.z, 15.0f);
    EXPECT_EQ(copiedHero.getComponent<RenderLayer>().order, 2);

    const auto &copiedCrate = copy.getGameObjects()[1];
    EXPECT_EQ(copiedCrate.getComponent<EntityInfo>().tag, "crate");
    EXPECT_EQ(copiedCrate.getComponent<EntityInfo>().width, 16);
    EXPECT_EQ(copiedCrate.getComponent<EntityInfo>().height, 16);
    EXPECT_FLOAT_EQ(copiedCrate.getComponent<Transform>().position.x, 80.0f);
    EXPECT_FLOAT_EQ(copiedCrate.getComponent<Transform>().position.y, 96.0f);
    EXPECT_EQ(copiedCrate.getComponent<RenderLayer>().order, 5);
}

TEST(SceneTests, RoundTripsSceneGravity)
{
    Scene original{"gravity scene"};
    original.setGravity({0.0f, -520.0f});

    YAML::Emitter serialized;
    original.serialize(serialized);
    ASSERT_TRUE(serialized.good());
    YAML::Node firstPass = YAML::Load(serialized.c_str());
    ASSERT_TRUE(firstPass["gravity scene"]["Gravity"].IsSequence());
    EXPECT_FLOAT_EQ(firstPass["gravity scene"]["Gravity"][0].as<float>(), 0.0f);
    EXPECT_FLOAT_EQ(firstPass["gravity scene"]["Gravity"][1].as<float>(), -520.0f);

    Scene copy{YAML::Load(serialized.c_str())};
    YAML::Emitter roundTripped;
    copy.serialize(roundTripped);
    YAML::Node secondPass = YAML::Load(roundTripped.c_str());
    EXPECT_FLOAT_EQ(secondPass["gravity scene"]["Gravity"][0].as<float>(), 0.0f);
    EXPECT_FLOAT_EQ(secondPass["gravity scene"]["Gravity"][1].as<float>(), -520.0f);
}
