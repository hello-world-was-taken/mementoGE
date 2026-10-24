#include "core/Scene.h"
#include "core/SystemGraph.h"
#include "core/SystemRegistry.h"
#include "core/SystemScheduler.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace
{
using namespace std::chrono_literals;

class SystemSchedulerModes : public ::testing::TestWithParam<std::size_t>
{
protected:
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
};

struct PublishedValue
{
    int value = 0;
};

struct UpdateState
{
    std::atomic<int> finished{0};
    int calls = 0;
    bool failUpdate = false;
    bool waitForRelease = false;
    std::shared_future<void> release;
    std::promise<void> started;
    std::thread::id updateThread;
    const FrameContext *observedFrame = nullptr;
    float observedDeltaTime = 0.0f;
    double observedElapsedTime = 0.0;
    KeyType observedKey = KeyType::None;
    bool observedEditMode = false;
};

class ProbeSystem final : public ISystem
{
public:
    explicit ProbeSystem(UpdateState &state, SystemThreadAffinity affinity = SystemThreadAffinity::AnyWorker)
        : m_state(state), m_affinity(affinity)
    {
    }

    void update(SystemContext &context) override
    {
        ++m_state.calls;
        m_state.updateThread = std::this_thread::get_id();
        if (m_state.waitForRelease)
        {
            m_state.started.set_value();
            if (m_state.release.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release update");
            }
        }
        m_state.observedFrame = &context.frame;
        m_state.observedDeltaTime = context.frame.deltaTime;
        m_state.observedElapsedTime = context.frame.elapsedTime;
        m_state.observedEditMode = context.editMode;
        if (!context.frame.input.events.empty())
        {
            m_state.observedKey = context.frame.input.events.front().keyType;
        }
        m_state.finished.fetch_add(1, std::memory_order_relaxed);
        if (m_state.failUpdate)
        {
            throw std::runtime_error("update failed");
        }
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Probe").withAffinity(m_affinity);
    }

private:
    UpdateState &m_state;
    SystemThreadAffinity m_affinity;
};

struct SceneUpdateState : UpdateState
{
    int consumerCalls = 0;
    int observedValue = 0;
    int observedFinished = 0;
    entt::entity entity = entt::null;
    std::thread::id consumerThread;
};

std::shared_ptr<SceneUpdateState> sceneUpdateState;

class ScheduledProducer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        const auto state = sceneUpdateState;
        ProbeSystem update{*state};
        update.update(context);
        state->entity = context.entities.create();
        context.entities.emplace<PublishedValue>(state->entity).value = 42;
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Producer")
            .writes<PublishedValue>()
            .requestsStructuralChanges()
            .before("SystemSchedulerTests.Consumer");
    }
};

class ScheduledConsumer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        const auto state = sceneUpdateState;
        ++state->consumerCalls;
        state->consumerThread = std::this_thread::get_id();
        state->observedValue = context.entities.get<const PublishedValue>(state->entity).value;
        state->observedFinished = state->finished.load(std::memory_order_relaxed);
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Consumer")
            .reads<PublishedValue>()
            .onMainThread();
    }
};

class ScheduledDestroyer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        const auto state = sceneUpdateState;
        context.entities.destroy(state->entity);
        state->finished.fetch_add(1, std::memory_order_relaxed);
        if (state->failUpdate)
        {
            throw std::runtime_error("destroying update failed");
        }
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Destroyer").requestsStructuralChanges();
    }
};

void addScheduledSystems(Scene &scene)
{
    auto &registry = SystemRegistry::instance();
    registry.registerSystem<ScheduledProducer>("SystemSchedulerTests.Producer");
    registry.registerSystem<ScheduledConsumer>("SystemSchedulerTests.Consumer");
    scene.addSystem("SystemSchedulerTests.Consumer");
    scene.addSystem("SystemSchedulerTests.Producer");
    scene.setAllSystemsEnabled(true);
}

struct SharedQueryComponent
{
    int value = 0;
};

struct FirstQueryComponent
{
    int value = 0;
};

struct SecondQueryComponent
{
    int value = 0;
};

struct ConcurrentQueryState
{
    std::promise<void> started;
    std::shared_future<void> release;
    std::thread::id callerThread;
    std::thread::id updateThread;
    bool editMode = false;
    int queryPasses = 0;
};

template <typename OwnComponent, typename OtherComponent> class ConcurrentQuerySystem final : public ISystem
{
public:
    ConcurrentQuerySystem(std::string name, ConcurrentQueryState &state) : m_name(std::move(name)), m_state(state)
    {
    }

    void update(SystemContext &context) override
    {
        m_state.updateThread = std::this_thread::get_id();
        const auto query = [&]
        {
            auto mixed = context.entities.view<const SharedQueryComponent, OwnComponent>();
            EXPECT_EQ(mixed.begin(), mixed.end());
            const auto &readOnly = std::as_const(context.entities);
            auto read = readOnly.template view<OwnComponent, SharedQueryComponent>();
            EXPECT_EQ(read.begin(), read.end());
            EXPECT_EQ(context.entities.try_get<OwnComponent>(entt::null), nullptr);
            EXPECT_EQ(readOnly.template try_get<SharedQueryComponent>(entt::null), nullptr);
            EXPECT_FALSE(context.entities.all_of<OwnComponent>(entt::null));
            EXPECT_FALSE((context.entities.any_of<OwnComponent, SharedQueryComponent>(entt::null)));
            EXPECT_THROW((void)context.entities.view<OtherComponent>(), std::logic_error);
            EXPECT_EQ(context.editMode, m_state.editMode);
            EXPECT_FLOAT_EQ(context.frame.deltaTime, 1.0f / 60.0f);
            ++m_state.queryPasses;
        };
        query();
        m_state.started.set_value();
        if (m_state.release.wait_for(5s) != std::future_status::ready)
        {
            throw std::runtime_error("test did not release concurrent update");
        }
        query();
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named(m_name).template reads<SharedQueryComponent>().template writes<OwnComponent>();
    }

private:
    std::string m_name;
    ConcurrentQueryState &m_state;
};

class GraphProbeSystem final : public ISystem
{
public:
    GraphProbeSystem(SystemDescriptor descriptor, std::function<void(SystemContext &)> update)
        : m_descriptor(std::move(descriptor)), m_update(std::move(update))
    {
    }

    void update(SystemContext &context) override
    {
        m_update(context);
    }

    SystemDescriptor descriptor() const override
    {
        return m_descriptor;
    }

private:
    SystemDescriptor m_descriptor;
    std::function<void(SystemContext &)> m_update;
};

using EnabledSystems = std::map<std::string, std::reference_wrapper<ISystem>>;

void expectWorkerThread(std::size_t workerCount, std::thread::id updateThread, std::thread::id caller)
{
    if (workerCount == 0)
    {
        EXPECT_EQ(updateThread, caller);
    }
    else
    {
        EXPECT_NE(updateThread, caller);
    }
}
} // namespace

TEST_P(SystemSchedulerModes, WorkerEligibleUpdatesRunOnWorkersWithAnInlineFallback)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    UpdateState state;
    ProbeSystem system{state};
    frame.deltaTime = 1.0f / 60.0f;
    frame.elapsedTime = 12.5;
    Event event;
    event.type = EventType::KeyPress;
    event.keyType = KeyType::W;
    frame.input.events.push_back(event);
    system.descriptor().prepareRegistry(registry);

    scheduler.run(system, registry, physics, frame, true);

    EXPECT_EQ(state.calls, 1);
    EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);
    expectWorkerThread(GetParam(), state.updateThread, std::this_thread::get_id());
    EXPECT_EQ(state.observedFrame, &frame);
    EXPECT_FLOAT_EQ(state.observedDeltaTime, frame.deltaTime);
    EXPECT_DOUBLE_EQ(state.observedElapsedTime, frame.elapsedTime);
    EXPECT_EQ(state.observedKey, KeyType::W);
    EXPECT_TRUE(state.observedEditMode);
}

TEST_P(SystemSchedulerModes, MainThreadUpdatesRemainOnTheCaller)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    UpdateState state;
    ProbeSystem system{state, SystemThreadAffinity::MainThread};
    system.descriptor().prepareRegistry(registry);

    scheduler.run(system, registry, physics, frame);

    EXPECT_EQ(state.calls, 1);
    EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(state.updateThread, std::this_thread::get_id());
}

TEST_P(SystemSchedulerModes, UpdateErrorsPropagateAndTheSchedulerCanRunAgain)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    for (const auto affinity : {SystemThreadAffinity::AnyWorker, SystemThreadAffinity::MainThread})
    {
        SCOPED_TRACE(affinity == SystemThreadAffinity::MainThread ? "main thread" : "worker");
        UpdateState state;
        ProbeSystem system{state, affinity};
        state.failUpdate = true;
        system.descriptor().prepareRegistry(registry);

        try
        {
            scheduler.run(system, registry, physics, frame);
            FAIL() << "Expected the update error";
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_STREQ(error.what(), "update failed");
        }
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);

        state.failUpdate = false;
        EXPECT_NO_THROW(scheduler.run(system, registry, physics, frame));
        EXPECT_EQ(state.calls, 2);
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 2);
    }
}

TEST_P(SystemSchedulerModes, RunWaitsForTheWholeUpdateBeforeReturningOrThrowing)
{
    for (bool failUpdate : {false, true})
    {
        SCOPED_TRACE(failUpdate);
        SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
        UpdateState state;
        ProbeSystem system{state};
        state.waitForRelease = true;
        state.failUpdate = failUpdate;
        std::promise<void> release;
        state.release = release.get_future().share();
        auto started = state.started.get_future();
        frame.deltaTime = 1.0f / 60.0f;
        frame.elapsedTime = 7.0;
        system.descriptor().prepareRegistry(registry);

        auto run = std::async(std::launch::async, [&]
            {
                scheduler.run(system, registry, physics, frame);
            });

        EXPECT_EQ(started.wait_for(5s), std::future_status::ready);
        EXPECT_EQ(run.wait_for(50ms), std::future_status::timeout);
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 0);
        release.set_value();
        EXPECT_EQ(run.wait_for(5s), std::future_status::ready);
        if (failUpdate)
        {
            EXPECT_THROW(run.get(), std::runtime_error);
        }
        else
        {
            EXPECT_NO_THROW(run.get());
        }
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);
        EXPECT_EQ(state.observedFrame, &frame);
        EXPECT_FLOAT_EQ(state.observedDeltaTime, frame.deltaTime);
        EXPECT_DOUBLE_EQ(state.observedElapsedTime, frame.elapsedTime);
    }
}

TEST_P(SystemSchedulerModes, SceneConsumersObserveCompletedUpdatesInGraphOrder)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    Scene scene{"scheduled scene"};
    addScheduledSystems(scene);
    frame.deltaTime = 1.0f / 60.0f;

    scene.play();
    scene.update(frame, scheduler);

    EXPECT_EQ(sceneUpdateState->calls, 1);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
    EXPECT_EQ(sceneUpdateState->observedFinished, 1);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 1);
    EXPECT_FLOAT_EQ(sceneUpdateState->observedDeltaTime, frame.deltaTime);
    expectWorkerThread(GetParam(), sceneUpdateState->updateThread, std::this_thread::get_id());
    EXPECT_EQ(sceneUpdateState->consumerThread, std::this_thread::get_id());
}

TEST(SystemSchedulerTests, IndependentInvocationsOwnTheirAccessScopesWhileQueryingPreparedPools)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    frame.deltaTime = 1.0f / 60.0f;
    SystemScheduler scheduler{JobSystemConfig{2, 8, true}};
    std::promise<void> release;
    const auto released = release.get_future().share();
    ConcurrentQueryState firstState;
    firstState.release = released;
    firstState.editMode = true;
    ConcurrentQueryState secondState;
    secondState.release = released;
    auto firstStarted = firstState.started.get_future();
    auto secondStarted = secondState.started.get_future();
    ConcurrentQuerySystem<FirstQueryComponent, SecondQueryComponent> first{"ConcurrentFirst", firstState};
    ConcurrentQuerySystem<SecondQueryComponent, FirstQueryComponent> second{"ConcurrentSecond", secondState};
    EXPECT_FALSE(first.descriptor().conflictsWith(second.descriptor()));
    EXPECT_EQ(std::as_const(registry).storage<SharedQueryComponent>(), nullptr);
    EXPECT_EQ(std::as_const(registry).storage<FirstQueryComponent>(), nullptr);
    EXPECT_EQ(std::as_const(registry).storage<SecondQueryComponent>(), nullptr);
    first.descriptor().prepareRegistry(registry);
    second.descriptor().prepareRegistry(registry);

    auto firstRun = std::async(std::launch::async, [&]
        {
            firstState.callerThread = std::this_thread::get_id();
            scheduler.run(first, registry, physics, frame, true);
        });
    auto secondRun = std::async(std::launch::async, [&]
        {
            secondState.callerThread = std::this_thread::get_id();
            scheduler.run(second, registry, physics, frame, false);
        });

    EXPECT_EQ(firstStarted.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(secondStarted.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(firstRun.wait_for(20ms), std::future_status::timeout);
    EXPECT_EQ(secondRun.wait_for(20ms), std::future_status::timeout);
    release.set_value();
    EXPECT_EQ(firstRun.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(secondRun.wait_for(5s), std::future_status::ready);
    EXPECT_NO_THROW(firstRun.get());
    EXPECT_NO_THROW(secondRun.get());

    EXPECT_EQ(firstState.queryPasses, 2);
    EXPECT_EQ(secondState.queryPasses, 2);
    EXPECT_NE(firstState.updateThread, firstState.callerThread);
    EXPECT_NE(secondState.updateThread, secondState.callerThread);
    EXPECT_NE(firstState.updateThread, secondState.updateThread);
    ASSERT_NE(std::as_const(registry).storage<SharedQueryComponent>(), nullptr);
    ASSERT_NE(std::as_const(registry).storage<FirstQueryComponent>(), nullptr);
    ASSERT_NE(std::as_const(registry).storage<SecondQueryComponent>(), nullptr);
    EXPECT_EQ(std::as_const(registry).storage<SharedQueryComponent>()->size(), 0u);
    EXPECT_EQ(std::as_const(registry).storage<FirstQueryComponent>()->size(), 0u);
    EXPECT_EQ(std::as_const(registry).storage<SecondQueryComponent>()->size(), 0u);
    const auto entities = registry.view<entt::entity>();
    EXPECT_EQ(entities.begin(), entities.end());
}

TEST(SystemSchedulerTests, StandaloneSceneUpdatesRunInline)
{
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    Scene scene{"standalone scheduled scene"};
    addScheduledSystems(scene);

    scene.update(FrameContext{}, true);

    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
    EXPECT_EQ(sceneUpdateState->observedFinished, 1);
    EXPECT_EQ(sceneUpdateState->updateThread, std::this_thread::get_id());
}

TEST(SystemSchedulerTests, SceneKeepsContextAliveAndIncludesTheFullUpdateInSystemTiming)
{
    SystemScheduler scheduler{JobSystemConfig{1, 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    const auto state = sceneUpdateState;
    state->waitForRelease = true;
    std::promise<void> release;
    state->release = release.get_future().share();
    auto started = state->started.get_future();
    Scene scene{"blocked system scene"};
    addScheduledSystems(scene);
    FrameContext frame;
    frame.deltaTime = 1.0f / 60.0f;

    auto update = std::async(std::launch::async, [&]
        {
            scene.update(frame, scheduler, true);
        });
    EXPECT_EQ(started.wait_for(5s), std::future_status::ready);
    const auto blockedSince = std::chrono::steady_clock::now();
    EXPECT_EQ(update.wait_for(50ms), std::future_status::timeout);
    const auto blockedTime = std::chrono::steady_clock::now() - blockedSince;
    release.set_value();
    EXPECT_EQ(update.wait_for(5s), std::future_status::ready);
    EXPECT_NO_THROW(update.get());

    EXPECT_EQ(state->observedValue, 42);
    EXPECT_FLOAT_EQ(state->observedDeltaTime, frame.deltaTime);
    const auto info = scene.getSystemInfo();
    ASSERT_EQ(info.size(), 2u);
    EXPECT_EQ(info.front().name, "SystemSchedulerTests.Producer");
    const double blockedMilliseconds = std::chrono::duration<double, std::milli>(blockedTime).count();
    EXPECT_GE(info.front().lastUpdateMilliseconds, blockedMilliseconds);
}

TEST_P(SystemSchedulerModes, SceneUpdateFailureSkipsDependentsAndAllowsAnotherUpdate)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    sceneUpdateState->failUpdate = true;
    Scene scene{"failing scheduled scene"};
    addScheduledSystems(scene);

    EXPECT_THROW(scene.update(FrameContext{}, scheduler, true), std::runtime_error);
    EXPECT_EQ(sceneUpdateState->calls, 1);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 0);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 1);

    sceneUpdateState->failUpdate = false;
    scene.update(FrameContext{}, scheduler, true);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
    EXPECT_EQ(sceneUpdateState->observedFinished, 2);
}

TEST_P(SystemSchedulerModes, ScenePauseAndDisablementDoNotLaunchSystemJobs)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    Scene scene{"disabled scheduled scene"};
    addScheduledSystems(scene);

    scene.update(FrameContext{}, scheduler);
    EXPECT_EQ(sceneUpdateState->calls, 0);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 0);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 0);

    scene.setAllSystemsEnabled(false);
    scene.update(FrameContext{}, scheduler, true);
    EXPECT_EQ(sceneUpdateState->calls, 0);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 0);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 0);

    scene.setAllSystemsEnabled(true);
    scene.play();
    scene.update(FrameContext{}, scheduler);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
}

TEST_P(SystemSchedulerModes, SceneCleansUpObjectsDestroyedByUpdatesBeforeReturningOrThrowing)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    SystemRegistry::instance().registerSystem<ScheduledDestroyer>("SystemSchedulerTests.Destroyer");
    for (bool failUpdate : {false, true})
    {
        SCOPED_TRACE(failUpdate);
        sceneUpdateState = std::make_shared<SceneUpdateState>();
        sceneUpdateState->failUpdate = failUpdate;
        Scene scene{"system destruction scene"};
        sceneUpdateState->entity = scene.addGameObject(8, 8, "removed by system").getEntityId();
        scene.addSystem("SystemSchedulerTests.Destroyer");
        scene.setAllSystemsEnabled(true);

        if (failUpdate)
        {
            EXPECT_THROW(scene.update(FrameContext{}, scheduler, true), std::runtime_error);
        }
        else
        {
            EXPECT_NO_THROW(scene.update(FrameContext{}, scheduler, true));
        }

        EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 1);
        EXPECT_TRUE(scene.getGameObjects().empty());
    }
}

TEST(SystemSchedulerTests, GraphWorkersOverlapWithIndependentScopesAndPreparedEmptyPools)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    frame.deltaTime = 1.0f / 60.0f;
    SystemScheduler scheduler{JobSystemConfig{2, 1, true}};
    std::promise<void> release;
    const auto released = release.get_future().share();
    ConcurrentQueryState firstState;
    firstState.release = released;
    ConcurrentQueryState secondState;
    secondState.release = released;
    auto firstStarted = firstState.started.get_future();
    auto secondStarted = secondState.started.get_future();
    ConcurrentQuerySystem<FirstQueryComponent, SecondQueryComponent> first{"ZFirst", firstState};
    ConcurrentQuerySystem<SecondQueryComponent, FirstQueryComponent> second{"ASecond", secondState};
    EXPECT_EQ(std::as_const(registry).storage<SharedQueryComponent>(), nullptr);
    EXPECT_EQ(std::as_const(registry).storage<FirstQueryComponent>(), nullptr);
    EXPECT_EQ(std::as_const(registry).storage<SecondQueryComponent>(), nullptr);
    first.descriptor().prepareRegistry(registry);
    second.descriptor().prepareRegistry(registry);
    const auto graph = SystemGraph::build({first.descriptor(), second.descriptor()});
    const EnabledSystems enabled{{"ZFirst", first}, {"ASecond", second}};
    std::thread::id caller;
    auto run = std::async(std::launch::async, [&]
        {
            caller = std::this_thread::get_id();
            return scheduler.run(graph, enabled, registry, physics, frame);
        });

    EXPECT_EQ(firstStarted.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(secondStarted.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(run.wait_for(20ms), std::future_status::timeout);
    release.set_value();
    EXPECT_EQ(run.wait_for(5s), std::future_status::ready);
    std::map<std::string, double> timings;
    EXPECT_NO_THROW(timings = run.get());

    EXPECT_EQ(firstState.queryPasses, 2);
    EXPECT_EQ(secondState.queryPasses, 2);
    EXPECT_NE(firstState.updateThread, caller);
    EXPECT_NE(secondState.updateThread, caller);
    EXPECT_NE(firstState.updateThread, secondState.updateThread);
    EXPECT_EQ(timings.size(), 2u);
    EXPECT_EQ(timings.count("ZFirst"), 1u);
    EXPECT_EQ(timings.count("ASecond"), 1u);
    EXPECT_EQ(std::as_const(registry).storage<SharedQueryComponent>()->size(), 0u);
    EXPECT_EQ(std::as_const(registry).storage<FirstQueryComponent>()->size(), 0u);
    EXPECT_EQ(std::as_const(registry).storage<SecondQueryComponent>()->size(), 0u);
}

TEST(SystemSchedulerTests, GraphFanInWaitsForBothCompletedProducers)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    SystemScheduler scheduler{JobSystemConfig{2, 1, true}};
    std::promise<void> firstStarted;
    std::promise<void> secondStarted;
    auto firstReady = firstStarted.get_future();
    auto secondReady = secondStarted.get_future();
    std::promise<void> releaseFirst;
    std::promise<void> releaseSecond;
    const auto firstReleased = releaseFirst.get_future().share();
    const auto secondReleased = releaseSecond.get_future().share();
    std::promise<void> firstFinished;
    auto firstDone = firstFinished.get_future();
    std::atomic<int> finished{0};
    std::atomic<int> consumerCalls{0};
    int observedFinished = 0;
    std::thread::id caller;
    std::thread::id consumerThread;
    GraphProbeSystem first{SystemDescriptor::named("First").before("Consumer"), [&](SystemContext &)
        {
            firstStarted.set_value();
            if (firstReleased.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release first producer");
            }
            finished.fetch_add(1, std::memory_order_relaxed);
            firstFinished.set_value();
        }};
    GraphProbeSystem second{SystemDescriptor::named("Second").before("Consumer"), [&](SystemContext &)
        {
            secondStarted.set_value();
            if (secondReleased.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release second producer");
            }
            finished.fetch_add(1, std::memory_order_relaxed);
        }};
    GraphProbeSystem consumer{SystemDescriptor::named("Consumer").onMainThread(), [&](SystemContext &)
        {
            observedFinished = finished.load(std::memory_order_relaxed);
            consumerThread = std::this_thread::get_id();
            consumerCalls.fetch_add(1, std::memory_order_relaxed);
        }};
    first.descriptor().prepareRegistry(registry);
    second.descriptor().prepareRegistry(registry);
    consumer.descriptor().prepareRegistry(registry);
    const auto graph = SystemGraph::build({consumer.descriptor(), first.descriptor(), second.descriptor()});
    const EnabledSystems enabled{{"First", first}, {"Second", second}, {"Consumer", consumer}};
    auto run = std::async(std::launch::async, [&]
        {
            caller = std::this_thread::get_id();
            return scheduler.run(graph, enabled, registry, physics, frame);
        });

    EXPECT_EQ(firstReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(secondReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(consumerCalls.load(std::memory_order_relaxed), 0);
    releaseFirst.set_value();
    EXPECT_EQ(firstDone.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(run.wait_for(20ms), std::future_status::timeout);
    EXPECT_EQ(consumerCalls.load(std::memory_order_relaxed), 0);
    releaseSecond.set_value();
    EXPECT_EQ(run.wait_for(5s), std::future_status::ready);
    std::map<std::string, double> timings;
    EXPECT_NO_THROW(timings = run.get());
    EXPECT_EQ(timings.size(), 3u);
    EXPECT_EQ(consumerCalls.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(observedFinished, 2);
    EXPECT_EQ(consumerThread, caller);
}

TEST(SystemSchedulerTests, ReadyMainThreadSystemsFinishBeforeAnyReadyWorkersStart)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    SystemScheduler scheduler{JobSystemConfig{2, 1, true}};
    const auto caller = std::this_thread::get_id();
    std::promise<void> workerStarted;
    auto workerReady = workerStarted.get_future();
    std::atomic<int> mainCalls{0};
    int observedMainCalls = 0;
    std::thread::id workerThread;
    std::vector<std::string> mainOrder;
    GraphProbeSystem worker{SystemDescriptor::named("Worker"), [&](SystemContext &)
        {
            workerThread = std::this_thread::get_id();
            observedMainCalls = mainCalls.load(std::memory_order_relaxed);
            workerStarted.set_value();
        }};
    const auto runMain = [&](const std::string &name)
    {
        EXPECT_EQ(std::this_thread::get_id(), caller);
        EXPECT_EQ(workerReady.wait_for(20ms), std::future_status::timeout);
        mainOrder.push_back(name);
        mainCalls.fetch_add(1, std::memory_order_relaxed);
    };
    GraphProbeSystem firstMain{SystemDescriptor::named("ZMain").onMainThread(),
        [&](SystemContext &) { runMain("ZMain"); }};
    GraphProbeSystem secondMain{SystemDescriptor::named("AMain").onMainThread(),
        [&](SystemContext &) { runMain("AMain"); }};
    worker.descriptor().prepareRegistry(registry);
    firstMain.descriptor().prepareRegistry(registry);
    secondMain.descriptor().prepareRegistry(registry);
    const auto graph = SystemGraph::build({worker.descriptor(), firstMain.descriptor(), secondMain.descriptor()});
    const EnabledSystems enabled{{"Worker", worker}, {"ZMain", firstMain}, {"AMain", secondMain}};

    const auto timings = scheduler.run(graph, enabled, registry, physics, frame);

    EXPECT_EQ(mainOrder, (std::vector<std::string>{"ZMain", "AMain"}));
    EXPECT_EQ(observedMainCalls, 2);
    EXPECT_NE(workerThread, caller);
    EXPECT_EQ(timings.size(), 3u);
}

TEST(SystemSchedulerTests, MainThreadGraphFailureStopsReadyWorkersAndAllowsRecovery)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    SystemScheduler scheduler{JobSystemConfig{2, 1, true}};
    bool failMain = true;
    int mainCalls = 0;
    int workerCalls = 0;
    const auto caller = std::this_thread::get_id();
    GraphProbeSystem worker{SystemDescriptor::named("Worker"), [&](SystemContext &)
        {
            EXPECT_NE(std::this_thread::get_id(), caller);
            ++workerCalls;
        }};
    GraphProbeSystem main{SystemDescriptor::named("Main").onMainThread(), [&](SystemContext &)
        {
            EXPECT_EQ(std::this_thread::get_id(), caller);
            ++mainCalls;
            if (failMain)
            {
                throw std::runtime_error("main update failed");
            }
        }};
    worker.descriptor().prepareRegistry(registry);
    main.descriptor().prepareRegistry(registry);
    const auto graph = SystemGraph::build({worker.descriptor(), main.descriptor()});
    const EnabledSystems enabled{{"Worker", worker}, {"Main", main}};

    EXPECT_THROW(scheduler.run(graph, enabled, registry, physics, frame), std::runtime_error);
    EXPECT_EQ(mainCalls, 1);
    EXPECT_EQ(workerCalls, 0);
    failMain = false;
    const auto timings = scheduler.run(graph, enabled, registry, physics, frame);
    EXPECT_EQ(mainCalls, 2);
    EXPECT_EQ(workerCalls, 1);
    EXPECT_EQ(timings.size(), 2u);
}

TEST(SystemSchedulerTests, GraphFailureDrainsRunningPeersSkipsSuccessorsAndAllowsRecovery)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    frame.deltaTime = 1.0f / 60.0f;
    SystemScheduler scheduler{JobSystemConfig{2, 1, true}};
    bool failUpdate = true;
    std::promise<void> peerStarted;
    const auto peerReady = peerStarted.get_future().share();
    std::promise<void> firstFailureProcessed;
    auto failureReady = firstFailureProcessed.get_future();
    std::promise<void> releasePeer;
    const auto released = releasePeer.get_future().share();
    std::atomic<int> firstCalls{0};
    std::atomic<int> peerFinished{0};
    std::atomic<int> successorCalls{0};
    GraphProbeSystem first{SystemDescriptor::named("ZFirst"), [&](SystemContext &)
        {
            firstCalls.fetch_add(1, std::memory_order_relaxed);
            if (failUpdate)
            {
                if (peerReady.wait_for(5s) != std::future_status::ready)
                {
                    throw std::runtime_error("peer update did not start");
                }
                throw std::runtime_error("first graph update failed");
            }
        }};
    GraphProbeSystem peer{SystemDescriptor::named("APeer"), [&](SystemContext &context)
        {
            if (failUpdate)
            {
                peerStarted.set_value();
                if (released.wait_for(5s) != std::future_status::ready)
                {
                    throw std::runtime_error("test did not release peer update");
                }
            }
            EXPECT_EQ(&context.frame, &frame);
            EXPECT_FLOAT_EQ(context.frame.deltaTime, frame.deltaTime);
            peerFinished.fetch_add(1, std::memory_order_relaxed);
        }};
    GraphProbeSystem marker{SystemDescriptor::named("Marker"), [&](SystemContext &)
        {
            if (failUpdate)
            {
                // The peer occupies the other worker until the test releases it.
                firstFailureProcessed.set_value();
            }
        }};
    GraphProbeSystem successor{SystemDescriptor::named("Successor").after("ZFirst").after("APeer"),
        [&](SystemContext &) { successorCalls.fetch_add(1, std::memory_order_relaxed); }};
    first.descriptor().prepareRegistry(registry);
    peer.descriptor().prepareRegistry(registry);
    marker.descriptor().prepareRegistry(registry);
    successor.descriptor().prepareRegistry(registry);
    const auto graph = SystemGraph::build(
        {first.descriptor(), peer.descriptor(), marker.descriptor(), successor.descriptor()});
    const EnabledSystems enabled{
        {"ZFirst", first}, {"APeer", peer}, {"Marker", marker}, {"Successor", successor}};
    auto run = std::async(std::launch::async, [&]
        {
            return scheduler.run(graph, enabled, registry, physics, frame);
        });

    EXPECT_EQ(peerReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(failureReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(run.wait_for(20ms), std::future_status::timeout);
    EXPECT_EQ(peerFinished.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(successorCalls.load(std::memory_order_relaxed), 0);
    releasePeer.set_value();
    EXPECT_EQ(run.wait_for(5s), std::future_status::ready);
    try
    {
        (void)run.get();
        ADD_FAILURE() << "Expected the graph update error";
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_STREQ(error.what(), "first graph update failed");
    }
    EXPECT_EQ(peerFinished.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(successorCalls.load(std::memory_order_relaxed), 0);

    failUpdate = false;
    const auto timings = scheduler.run(graph, enabled, registry, physics, frame);
    EXPECT_EQ(timings.size(), 4u);
    EXPECT_EQ(firstCalls.load(std::memory_order_relaxed), 2);
    EXPECT_EQ(peerFinished.load(std::memory_order_relaxed), 2);
    EXPECT_EQ(successorCalls.load(std::memory_order_relaxed), 1);
}

TEST(SystemSchedulerTests, GraphErrorsUseStableSystemOrderRatherThanMapOrCompletionOrder)
{
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    SystemScheduler scheduler{JobSystemConfig{2, 1, true}};
    std::promise<void> firstStarted;
    auto firstReady = firstStarted.get_future();
    std::promise<void> laterFailureProcessed;
    auto failureReady = laterFailureProcessed.get_future();
    std::promise<void> releaseFirst;
    const auto released = releaseFirst.get_future().share();
    std::atomic<int> failed{0};
    std::atomic<int> successorCalls{0};
    GraphProbeSystem first{SystemDescriptor::named("ZFirst"), [&](SystemContext &)
        {
            firstStarted.set_value();
            if (released.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release first failing update");
            }
            failed.fetch_add(1, std::memory_order_relaxed);
            throw std::runtime_error("first system error");
        }};
    GraphProbeSystem second{SystemDescriptor::named("ASecond"), [&](SystemContext &)
        {
            failed.fetch_add(1, std::memory_order_relaxed);
            throw std::runtime_error("second system error");
        }};
    GraphProbeSystem marker{SystemDescriptor::named("Marker"), [&](SystemContext &)
        {
            // The first worker remains blocked, so the second failure is published.
            laterFailureProcessed.set_value();
        }};
    GraphProbeSystem successor{SystemDescriptor::named("Successor").after("ZFirst"),
        [&](SystemContext &) { successorCalls.fetch_add(1, std::memory_order_relaxed); }};
    first.descriptor().prepareRegistry(registry);
    second.descriptor().prepareRegistry(registry);
    marker.descriptor().prepareRegistry(registry);
    successor.descriptor().prepareRegistry(registry);
    const auto graph = SystemGraph::build(
        {first.descriptor(), second.descriptor(), marker.descriptor(), successor.descriptor()});
    const EnabledSystems enabled{
        {"ZFirst", first}, {"ASecond", second}, {"Marker", marker}, {"Successor", successor}};
    auto run = std::async(std::launch::async, [&]
        {
            return scheduler.run(graph, enabled, registry, physics, frame);
        });

    EXPECT_EQ(firstReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(failureReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(run.wait_for(20ms), std::future_status::timeout);
    releaseFirst.set_value();
    EXPECT_EQ(run.wait_for(5s), std::future_status::ready);
    try
    {
        (void)run.get();
        ADD_FAILURE() << "Expected the first system error";
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_STREQ(error.what(), "first system error");
    }
    EXPECT_EQ(failed.load(std::memory_order_relaxed), 2);
    EXPECT_EQ(successorCalls.load(std::memory_order_relaxed), 0);
}

TEST_P(SystemSchedulerModes, ZeroWorkersAndEditModeFollowTheStableSerialSequence)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 2, true}};
    for (bool editMode : {false, true})
    {
        if (!editMode && GetParam() != 0)
        {
            continue;
        }
        SCOPED_TRACE(editMode);
        std::vector<std::string> observed;
        const auto caller = std::this_thread::get_id();
        const auto record = [&](const std::string &name, SystemContext &context)
        {
            observed.push_back(name);
            EXPECT_EQ(&context.frame, &frame);
            EXPECT_EQ(context.editMode, editMode);
            expectWorkerThread(GetParam(), std::this_thread::get_id(), caller);
        };
        GraphProbeSystem dependent{SystemDescriptor::named("Dependent").after("Producer"),
            [&](SystemContext &context) { record("Dependent", context); }};
        GraphProbeSystem producer{SystemDescriptor::named("Producer"),
            [&](SystemContext &context) { record("Producer", context); }};
        GraphProbeSystem unrelated{SystemDescriptor::named("Unrelated"),
            [&](SystemContext &context) { record("Unrelated", context); }};
        dependent.descriptor().prepareRegistry(registry);
        producer.descriptor().prepareRegistry(registry);
        unrelated.descriptor().prepareRegistry(registry);
        const auto graph = SystemGraph::build({dependent.descriptor(), producer.descriptor(), unrelated.descriptor()});
        const EnabledSystems enabled{{"Dependent", dependent}, {"Producer", producer}, {"Unrelated", unrelated}};

        const auto timings = scheduler.run(graph, enabled, registry, physics, frame, editMode);

        EXPECT_EQ(observed, graph.orderedSystems());
        EXPECT_EQ(observed, (std::vector<std::string>{"Producer", "Dependent", "Unrelated"}));
        ASSERT_EQ(timings.size(), 3u);
        for (const auto &name : observed)
        {
            EXPECT_GE(timings.at(name), 0.0);
        }
    }
}

TEST_P(SystemSchedulerModes, GraphConflictsWaitForWholeComponentResourceAndStructuralUpdates)
{
    const std::vector<std::pair<SystemDescriptor, SystemDescriptor>> conflicts{
        {SystemDescriptor::named("ZFirst").writes<SharedQueryComponent>(),
            SystemDescriptor::named("ASecond").reads<SharedQueryComponent>()},
        {SystemDescriptor::named("ZFirst").writes<SharedQueryComponent>(),
            SystemDescriptor::named("ASecond").writes<SharedQueryComponent>()},
        {SystemDescriptor::named("ZFirst").writesResource(SystemResource::AudioDevice),
            SystemDescriptor::named("ASecond").readsResource(SystemResource::AudioDevice)},
        {SystemDescriptor::named("ZFirst").requestsStructuralChanges(),
            SystemDescriptor::named("ASecond").reads<SharedQueryComponent>()}};
    std::size_t conflictIndex = 0;
    for (const auto &[firstDescriptor, secondDescriptor] : conflicts)
    {
        SCOPED_TRACE(conflictIndex++);
        SystemScheduler scheduler{JobSystemConfig{GetParam(), 1, true}};
        std::promise<void> firstStarted;
        auto firstReady = firstStarted.get_future();
        std::promise<void> release;
        const auto released = release.get_future().share();
        std::atomic<bool> firstFinished{false};
        std::atomic<bool> observedFirstFinished{false};
        std::atomic<int> secondCalls{0};
        GraphProbeSystem first{firstDescriptor, [&](SystemContext &)
            {
                firstStarted.set_value();
                if (released.wait_for(5s) != std::future_status::ready)
                {
                    throw std::runtime_error("test did not release conflicting update");
                }
                firstFinished.store(true, std::memory_order_relaxed);
            }};
        GraphProbeSystem second{secondDescriptor, [&](SystemContext &)
            {
                observedFirstFinished.store(firstFinished.load(std::memory_order_relaxed), std::memory_order_relaxed);
                secondCalls.fetch_add(1, std::memory_order_relaxed);
            }};
        first.descriptor().prepareRegistry(registry);
        second.descriptor().prepareRegistry(registry);
        const auto graph = SystemGraph::build({first.descriptor(), second.descriptor()});
        const EnabledSystems enabled{{"ZFirst", first}, {"ASecond", second}};
        auto run = std::async(std::launch::async, [&]
            {
                return scheduler.run(graph, enabled, registry, physics, frame);
            });

        EXPECT_EQ(firstReady.wait_for(2s), std::future_status::ready);
        EXPECT_EQ(run.wait_for(20ms), std::future_status::timeout);
        EXPECT_EQ(secondCalls.load(std::memory_order_relaxed), 0);
        release.set_value();
        EXPECT_EQ(run.wait_for(5s), std::future_status::ready);
        std::map<std::string, double> timings;
        EXPECT_NO_THROW(timings = run.get());
        EXPECT_EQ(timings.size(), 2u);
        EXPECT_EQ(secondCalls.load(std::memory_order_relaxed), 1);
        EXPECT_TRUE(observedFirstFinished.load(std::memory_order_relaxed));
    }
}

TEST_P(SystemSchedulerModes, AllMainThreadAndDisabledNodesAdvanceTheOriginalDependencyChain)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 1, true}};
    const auto caller = std::this_thread::get_id();
    std::vector<std::string> observed;
    GraphProbeSystem first{SystemDescriptor::named("ZFirst").onMainThread().before("Disabled"),
        [&](SystemContext &)
        {
            EXPECT_EQ(std::this_thread::get_id(), caller);
            observed.push_back("ZFirst");
        }};
    GraphProbeSystem last{SystemDescriptor::named("ALast").onMainThread().after("Disabled"),
        [&](SystemContext &)
        {
            EXPECT_EQ(std::this_thread::get_id(), caller);
            observed.push_back("ALast");
        }};
    first.descriptor().prepareRegistry(registry);
    last.descriptor().prepareRegistry(registry);
    const auto graph = SystemGraph::build(
        {last.descriptor(), SystemDescriptor::named("Disabled"), first.descriptor()});
    const EnabledSystems enabled{{"ZFirst", first}, {"ALast", last}};

    const auto timings = scheduler.run(graph, enabled, registry, physics, frame);

    EXPECT_EQ(observed, (std::vector<std::string>{"ZFirst", "ALast"}));
    EXPECT_EQ(timings.size(), 2u);
    EXPECT_EQ(timings.count("ZFirst"), 1u);
    EXPECT_EQ(timings.count("ALast"), 1u);
    EXPECT_EQ(timings.count("Disabled"), 0u);
    EXPECT_TRUE(scheduler.run(graph, EnabledSystems{}, registry, physics, frame).empty());
    EXPECT_EQ(observed.size(), 2u);
    EXPECT_TRUE(scheduler.run(SystemGraph::build({}), EnabledSystems{}, registry, physics, frame).empty());
}

INSTANTIATE_TEST_SUITE_P(WorkerCounts, SystemSchedulerModes, ::testing::Values(0u, 1u, 2u));
