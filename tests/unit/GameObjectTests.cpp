#include "engine/core/GameObject.h"
#include "engine/core/Physics2D.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/ImportedGroup.h"
#include "engine/core/components/RenderLayer.h"
#include "engine/core/components/ShaderEffectRequest.h"
#include "engine/core/components/TileMapPlacement.h"
#include "engine/core/components/Transform.h"

#include <entt/entt.hpp>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <yaml-cpp/yaml.h>

namespace
{
YAML::Node serializeToNode(GameObject &gameObject)
{
    YAML::Emitter out;
    gameObject.serialize(out);
    EXPECT_TRUE(out.good()) << "YAML emitter failed: " << out.GetLastError();
    return YAML::Load(out.c_str());
}
} // namespace

TEST(GameObjectTests, SerializesTaggedGameObject)
{
    entt::registry registry;
    GameObject gameObject{registry};

    auto &info = gameObject.getComponent<EntityInfo>();
    info.tag = "hero";
    info.width = 32;
    info.height = 48;

    auto &transform = gameObject.getComponent<Transform>();
    transform.position = {12.0f, 34.0f, 0.0f};
    transform.rotation = {0.0f, 0.0f, 15.0f};
    transform.scale = {2.0f, 3.0f, 1.0f};

    const auto root = serializeToNode(gameObject);

    ASSERT_TRUE(root.IsMap());
    EXPECT_EQ(root.size(), 1);
    ASSERT_TRUE(root["hero"].IsDefined());

    const auto components = root["hero"];
    ASSERT_TRUE(components["EntityInfo"].IsDefined());
    ASSERT_TRUE(components["Transform"].IsDefined());
    ASSERT_TRUE(components["RenderLayer"].IsDefined());

    EXPECT_EQ(components["EntityInfo"]["Tag"].as<std::string>(), "hero");
    EXPECT_EQ(components["EntityInfo"]["Width"].as<unsigned int>(), 32);
    EXPECT_EQ(components["EntityInfo"]["Height"].as<unsigned int>(), 48);
    EXPECT_FLOAT_EQ(components["Transform"]["Position"][0].as<float>(), 12.0f);
    EXPECT_FLOAT_EQ(components["Transform"]["Rotation"][2].as<float>(), 15.0f);
    EXPECT_FLOAT_EQ(components["Transform"]["Scale"][1].as<float>(), 3.0f);
}

TEST(GameObjectTests, DeserializesTaggedGameObject)
{
    const char *yaml = R"(
hero:
  EntityInfo:
    UUID: 42
    Tag: hero
    Width: 20
    Height: 30
  Transform:
    Position: [1.5, 2.5, 3.5]
    Rotation: [4.0, 5.0, 6.0]
    Scale: [1.0, 2.0, 3.0]
  RenderLayer:
    Layer: Foreground
    Order: 7
    Mask: 255
)";

    entt::registry registry;
    Physics2D physics{{0.0f, 0.0f}};
    GameObject gameObject{registry, YAML::Load(yaml), physics};

    const auto &info = gameObject.getComponent<EntityInfo>();
    EXPECT_EQ(info.uuid, 42);
    EXPECT_EQ(info.tag, "hero");
    EXPECT_EQ(info.width, 20);
    EXPECT_EQ(info.height, 30);

    const auto &transform = gameObject.getComponent<Transform>();
    EXPECT_FLOAT_EQ(transform.position.x, 1.5f);
    EXPECT_FLOAT_EQ(transform.position.y, 2.5f);
    EXPECT_FLOAT_EQ(transform.position.z, 3.5f);
    EXPECT_FLOAT_EQ(transform.rotation.z, 6.0f);
    EXPECT_FLOAT_EQ(transform.scale.y, 2.0f);

    const auto &renderLayer = gameObject.getComponent<RenderLayer>();
    EXPECT_EQ(renderLayer.layer, RenderLayerType::Foreground);
    EXPECT_EQ(renderLayer.order, 7);
    EXPECT_EQ(renderLayer.mask, 255);
}

TEST(GameObjectTests, MigratesLegacyTileMapChildToPlacementMetadata)
{
    const YAML::Node legacy = YAML::Load(R"(
tree:
  EntityInfo: {Tag: tree, Width: 32, Height: 32}
  Transform: {Position: [0, 0, 0], Rotation: [0, 0, 0], Scale: [1, 1, 1]}
  TileMapChild:
    ParentUUID: 42
    ModelPath: models/tree.yaml
    SnappedToGrid: true
    Column: 3
    Row: 4
)");
    entt::registry registry;
    Physics2D physics{{0.0f, 0.0f}};
    GameObject gameObject{registry, legacy, physics};

    ASSERT_TRUE(gameObject.hasComponent<TileMapPlacement>());
    const TileMapPlacement &placement = gameObject.getComponent<TileMapPlacement>();
    EXPECT_EQ(placement.modelPath, "models/tree.yaml");
    EXPECT_TRUE(placement.snappedToGrid);
    EXPECT_EQ(placement.column, 3);
    EXPECT_EQ(placement.row, 4);

    const YAML::Node migrated = serializeToNode(gameObject);
    EXPECT_TRUE(migrated["tree"]["TileMapPlacement"]);
    EXPECT_FALSE(migrated["tree"]["TileMapChild"]);
}

TEST(GameObjectTests, RoundTripsThroughTaggedYaml)
{
    entt::registry registry;
    GameObject original{registry};

    original.getComponent<EntityInfo>().tag = "crate";
    original.getComponent<EntityInfo>().width = 16;
    original.getComponent<EntityInfo>().height = 24;
    original.getComponent<Transform>().position = {8.0f, 9.0f, 10.0f};
    original.getComponent<RenderLayer>().order = 3;

    YAML::Emitter out;
    original.serialize(out);
    ASSERT_TRUE(out.good()) << "YAML emitter failed: " << out.GetLastError();

    entt::registry secondRegistry;
    Physics2D physics{{0.0f, 0.0f}};
    GameObject copy{secondRegistry, YAML::Load(out.c_str()), physics};

    EXPECT_EQ(copy.getComponent<EntityInfo>().tag, "crate");
    EXPECT_EQ(copy.getComponent<EntityInfo>().width, 16);
    EXPECT_EQ(copy.getComponent<EntityInfo>().height, 24);
    EXPECT_FLOAT_EQ(copy.getComponent<Transform>().position.x, 8.0f);
    EXPECT_FLOAT_EQ(copy.getComponent<Transform>().position.y, 9.0f);
    EXPECT_EQ(copy.getComponent<RenderLayer>().order, 3);
}

TEST(GameObjectTests, RoundTripsImportedGroupMetadata)
{
    entt::registry registry;
    GameObject original{registry};
    original.getComponent<EntityInfo>().tag = "imported_tree";
    original.addComponent<ImportedGroup>();
    ImportedGroup &group = original.getComponent<ImportedGroup>();
    group.groupId = 9876;
    group.sourceAsset = "assets/models/jungle.yaml";

    YAML::Emitter out;
    original.serialize(out);
    ASSERT_TRUE(out.good()) << "YAML emitter failed: " << out.GetLastError();

    entt::registry secondRegistry;
    Physics2D physics{{0.0f, 0.0f}};
    GameObject copy{secondRegistry, YAML::Load(out.c_str()), physics};

    ASSERT_TRUE(copy.hasComponent<ImportedGroup>());
    EXPECT_EQ(copy.getComponent<ImportedGroup>().groupId, 9876);
    EXPECT_EQ(copy.getComponent<ImportedGroup>().sourceAsset, "assets/models/jungle.yaml");
}

TEST(GameObjectTests, RoundTripsShaderEffectUniforms)
{
    entt::registry registry;
    GameObject original{registry};
    original.getComponent<EntityInfo>().tag = "distorted_sprite";

    original.addComponent<ShaderEffectRequest>();
    ShaderEffectRequest &effect = original.getComponent<ShaderEffectRequest>();
    effect.vertexShaderPath = "shaders/effect.vert";
    effect.fragmentShaderPath = "shaders/effect.frag";
    effect.uniforms["u_Mode"] = 3;
    effect.uniforms["u_Strength"] = 0.25F;
    effect.uniforms["u_Offset"] = glm::vec2{1.0F, 2.0F};
    effect.uniforms["u_Direction"] = glm::vec3{3.0F, 4.0F, 5.0F};
    effect.uniforms["u_Tint"] = glm::vec4{0.1F, 0.2F, 0.3F, 0.4F};

    glm::mat4 transform{1.0F};
    for (glm::length_t column = 0; column < 4; ++column)
    {
        for (glm::length_t row = 0; row < 4; ++row)
        {
            transform[column][row] = static_cast<float>(column * 4 + row + 1);
        }
    }
    effect.uniforms["u_Transform"] = transform;

    const YAML::Node serialized = serializeToNode(original);
    ASSERT_EQ(serialized["distorted_sprite"]["ShaderEffectRequest"]["uniforms"]["u_Tint"]["type"].as<std::string>(),
        "vec4");

    entt::registry secondRegistry;
    Physics2D physics{{0.0F, 0.0F}};
    GameObject copy{secondRegistry, serialized, physics};

    ASSERT_TRUE(copy.hasComponent<ShaderEffectRequest>());
    const ShaderEffectRequest &loaded = copy.getComponent<ShaderEffectRequest>();
    EXPECT_EQ(loaded.vertexShaderPath, effect.vertexShaderPath);
    EXPECT_EQ(loaded.fragmentShaderPath, effect.fragmentShaderPath);
    ASSERT_EQ(loaded.uniforms.size(), effect.uniforms.size());
    EXPECT_EQ(std::get<int>(loaded.uniforms.at("u_Mode")), 3);
    EXPECT_FLOAT_EQ(std::get<float>(loaded.uniforms.at("u_Strength")), 0.25F);
    EXPECT_EQ(std::get<glm::vec2>(loaded.uniforms.at("u_Offset")), glm::vec2(1.0F, 2.0F));
    EXPECT_EQ(std::get<glm::vec3>(loaded.uniforms.at("u_Direction")), glm::vec3(3.0F, 4.0F, 5.0F));
    EXPECT_EQ(std::get<glm::vec4>(loaded.uniforms.at("u_Tint")), glm::vec4(0.1F, 0.2F, 0.3F, 0.4F));

    const glm::mat4 &loadedTransform = std::get<glm::mat4>(loaded.uniforms.at("u_Transform"));
    for (glm::length_t column = 0; column < 4; ++column)
    {
        for (glm::length_t row = 0; row < 4; ++row)
        {
            EXPECT_FLOAT_EQ(loadedTransform[column][row], transform[column][row]);
        }
    }
}

TEST(GameObjectTests, RejectsUntaggedComponentMap)
{
    const char *yaml = R"(
EntityInfo:
  Tag: broken
  Width: 1
  Height: 1
Transform:
  Position: [0, 0, 0]
  Rotation: [0, 0, 0]
  Scale: [1, 1, 1]
)";

    entt::registry registry;
    Physics2D physics{{0.0f, 0.0f}};
    const YAML::Node node = YAML::Load(yaml);

    EXPECT_THROW(GameObject(registry, node, physics), std::runtime_error);
}
