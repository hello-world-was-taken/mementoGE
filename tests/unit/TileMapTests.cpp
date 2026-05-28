#include "engine/core/TileMap.h"
#include "engine/core/Physics2D.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/ImportedGroup.h"
#include "engine/core/components/TileMapPlacement.h"
#include "engine/core/components/Transform.h"

#include <gtest/gtest.h>
#include <vector>
#include <yaml-cpp/yaml.h>

namespace
{
const char *tileMapAsset = R"(
TileMap:
  Name: jungle
  Columns: 2
  Rows: 2
  TileWidth: 128
  TileHeight: 128
  Objects:
    - tree:
        EntityInfo: {UUID: 13, Tag: tree, Width: 64, Height: 96}
        Transform:
          Position: [20, -10, 0]
          Rotation: [0, 0, 0]
          Scale: [1, 1, 1]
        TileMapPlacement: {ModelPath: tree.yaml, SnappedToGrid: true, Column: 1, Row: 0}
)";
}

TEST(TileMapTests, LoadsStandaloneAssetDocument)
{
    TileMap tileMap;
    std::vector<YAML::Node> objects;
    ASSERT_TRUE(tileMap.load(YAML::Load(tileMapAsset), objects));
    EXPECT_EQ(tileMap.name, "jungle");
    EXPECT_EQ(tileMap.columns, 2);
    EXPECT_EQ(tileMap.rows, 2);
    EXPECT_EQ(tileMap.tileWidth, 128);
    EXPECT_EQ(tileMap.tileHeight, 128);
    ASSERT_EQ(objects.size(), 1);
    EXPECT_TRUE(objects.front()["tree"]);
}

TEST(TileMapTests, FlattensObjectsThroughSceneFactory)
{
    entt::registry registry;
    Physics2D physics{{0.0f, 0.0f}};
    std::vector<GameObject> sceneObjects;
    TileMapImportContext context;
    context.importTransform.position = {100.0f, 50.0f, 0.0f};
    context.sourceAsset = "assets/models/jungle.yaml";
    context.createSceneObject =
        [&](const YAML::Node &node) -> GameObject &
        {
            sceneObjects.emplace_back(registry, node, physics);
            return sceneObjects.back();
        };

    const uint64_t groupId = TileMap::flattenIntoScene(YAML::Load(tileMapAsset), context);

    ASSERT_NE(groupId, 0);
    ASSERT_EQ(sceneObjects.size(), 1);
    GameObject &object = sceneObjects.front();
    EXPECT_FALSE(object.hasComponent<TileMapPlacement>());
    EXPECT_FLOAT_EQ(object.getComponent<Transform>().position.x, 120.0f);
    EXPECT_FLOAT_EQ(object.getComponent<Transform>().position.y, 40.0f);
    ASSERT_TRUE(object.hasComponent<ImportedGroup>());
    EXPECT_EQ(object.getComponent<ImportedGroup>().groupId, groupId);
    EXPECT_EQ(object.getComponent<ImportedGroup>().sourceAsset, "assets/models/jungle.yaml");
}

TEST(TileMapTests, ReadsLegacyComponentAssetsForMigration)
{
    const YAML::Node legacy = YAML::Load(R"(
old_map:
  EntityInfo: {Tag: old_map, Width: 32, Height: 32}
  Transform: {Position: [0, 0, 0], Rotation: [0, 0, 0], Scale: [1, 1, 1]}
  TileMap: {Columns: 1, Rows: 1, TileWidth: 32, TileHeight: 32}
  TileMapChildren: []
)");
    TileMap tileMap;
    std::vector<YAML::Node> objects;
    ASSERT_TRUE(tileMap.load(legacy, objects));
    EXPECT_EQ(tileMap.name, "old_map");
    EXPECT_EQ(tileMap.columns, 1);
    EXPECT_TRUE(objects.empty());
}
