#include "core/components/TileMapPlacement.h"

void TileMapPlacement::serialize(YAML::Emitter &out) const
{
    out << YAML::Key << "TileMapPlacement" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "ModelPath" << YAML::Value << modelPath;
    out << YAML::Key << "SnappedToGrid" << YAML::Value << snappedToGrid;
    out << YAML::Key << "Column" << YAML::Value << column;
    out << YAML::Key << "Row" << YAML::Value << row;
    out << YAML::EndMap;
}

void TileMapPlacement::deserialize(const YAML::Node &in)
{
    // Continue reading the former component name so existing tile-map assets
    // migrate to TileMapPlacement the next time they are saved.
    const YAML::Node node = in["TileMapPlacement"] ? in["TileMapPlacement"] : in["TileMapChild"];
    modelPath = node["ModelPath"] ? node["ModelPath"].as<std::string>() : std::string{};
    snappedToGrid = node["SnappedToGrid"] ? node["SnappedToGrid"].as<bool>() : true;
    column = node["Column"] ? node["Column"].as<int>() : -1;
    row = node["Row"] ? node["Row"].as<int>() : -1;
}
