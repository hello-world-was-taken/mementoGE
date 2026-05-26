#include "engine/core/Scene.h"
#include "engine/core/SystemRegistry.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/RenderLayer.h"
#include "engine/core/components/Transform.h"

#include <gtest/gtest.h>
#include <string>
#include <yaml-cpp/yaml.h>

namespace
{
class CountingSystem : public ISystem
{
public:
    void update(entt::registry &) override
    {
        ++updateCount;
    }

    void onDisabled(entt::registry &) override
    {
        ++disabledCount;
    }

    static int updateCount;
    static int disabledCount;
};

int CountingSystem::updateCount = 0;
int CountingSystem::disabledCount = 0;

YAML::Node serializeToNode(Scene &scene)
{
    YAML::Emitter out;
    scene.serialize(out);
    EXPECT_TRUE(out.good()) << "YAML emitter failed: " << out.GetLastError();
    return YAML::Load(out.c_str());
}
} // namespace

TEST(SceneTests, DisabledSystemDoesNotUpdateAndOverrideIsNotSerialized)
{
    constexpr const char *systemName = "SceneTests.CountingSystem";
    SystemRegistry::instance().registerSystem<CountingSystem>(systemName);
    CountingSystem::updateCount = 0;
    CountingSystem::disabledCount = 0;

    Scene scene{"system test"};
    scene.addSystem(systemName);
    scene.play();
    scene.update();
    EXPECT_EQ(CountingSystem::updateCount, 1);

    ASSERT_TRUE(scene.setSystemEnabled(systemName, false));
    EXPECT_EQ(CountingSystem::disabledCount, 1);
    scene.update();
    EXPECT_EQ(CountingSystem::updateCount, 1);

    const auto info = scene.getSystemInfo();
    ASSERT_EQ(info.size(), 1u);
    EXPECT_FALSE(info.front().enabled);

    Scene clone = scene.clone("system test clone");
    const auto clonedInfo = clone.getSystemInfo();
    ASSERT_EQ(clonedInfo.size(), 1u);
    EXPECT_TRUE(clonedInfo.front().enabled);
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
