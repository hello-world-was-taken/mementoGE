#pragma once

#include <string>
#include <yaml-cpp/yaml.h>

// Editor-only authoring metadata for an object placed in a tile-map asset.
// This component is removed when the tile map is flattened into a game scene.
struct TileMapPlacement
{
    std::string modelPath;
    bool snappedToGrid = true;
    int column = -1;
    int row = -1;

    void serialize(YAML::Emitter &out) const;
    void deserialize(const YAML::Node &in);
};
