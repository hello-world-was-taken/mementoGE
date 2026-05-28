#pragma once

#include "core/GameObject.h"
#include "core/components/Transform.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>

struct TileMapImportContext
{
    Transform importTransform;
    std::string sourceAsset;
    std::function<GameObject &(const YAML::Node &)> createSceneObject;
};

// A TileMap is an authoring asset, not an ECS component. Its objects live in
// the Tile Map Editor's private Scene and are flattened into ordinary objects
// when the asset is imported into a game Scene.
class TileMap
{
public:
    std::string name;
    int columns = 16;
    int rows = 9;
    int tileWidth = 128;
    int tileHeight = 128;

    bool load(const YAML::Node &asset, std::vector<YAML::Node> &objects);
    bool serialize(YAML::Emitter &out, const std::vector<GameObject> &objects) const;
    bool hasValidLayout() const;

    static bool isTileMapAsset(const YAML::Node &asset);
    static uint64_t flattenIntoScene(const YAML::Node &asset, const TileMapImportContext &context);
    static Transform transformFromModelMatrix(const glm::mat4 &modelMatrix);
};
