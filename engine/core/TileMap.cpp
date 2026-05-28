#include "core/TileMap.h"

#include "core/components/EntityInfo.h"
#include "core/components/ImportedGroup.h"

#include <cmath>

namespace
{
Transform transformFromComponents(const YAML::Node &components)
{
    Transform transform;
    const YAML::Node node = components["Transform"];
    if (node["Position"])
    {
        transform.position = {node["Position"][0].as<float>(),
            node["Position"][1].as<float>(),
            node["Position"][2].as<float>()};
    }
    if (node["Rotation"])
    {
        transform.rotation = {node["Rotation"][0].as<float>(),
            node["Rotation"][1].as<float>(),
            node["Rotation"][2].as<float>()};
    }
    if (node["Scale"])
    {
        transform.scale =
            {node["Scale"][0].as<float>(), node["Scale"][1].as<float>(), node["Scale"][2].as<float>()};
    }
    return transform;
}

void writeTransform(YAML::Node components, const Transform &transform)
{
    components["Transform"]["Position"] =
        std::vector<float>{transform.position.x, transform.position.y, transform.position.z};
    components["Transform"]["Rotation"] =
        std::vector<float>{transform.rotation.x, transform.rotation.y, transform.rotation.z};
    components["Transform"]["Scale"] =
        std::vector<float>{transform.scale.x, transform.scale.y, transform.scale.z};
}
} // namespace

bool TileMap::isTileMapAsset(const YAML::Node &asset)
{
    if (!asset || !asset.IsMap())
    {
        return false;
    }
    if (asset["TileMap"] && asset["TileMap"].IsMap())
    {
        return true;
    }
    return asset.size() == 1 && asset.begin()->second["TileMap"];
}

bool TileMap::load(const YAML::Node &asset, std::vector<YAML::Node> &objects)
{
    objects.clear();
    if (!isTileMapAsset(asset))
    {
        return false;
    }

    YAML::Node data;
    YAML::Node serializedObjects;
    if (asset["TileMap"] && asset["TileMap"].IsMap())
    {
        data = asset["TileMap"];
        name = data["Name"] ? data["Name"].as<std::string>() : "tile_map";
        serializedObjects = data["Objects"];
    }
    else
    {
        name = asset.begin()->first.as<std::string>();
        const YAML::Node components = asset.begin()->second;
        data = components["TileMap"];
        serializedObjects = components["TileMapChildren"];
    }

    columns = data["Columns"] ? data["Columns"].as<int>() : 16;
    rows = data["Rows"] ? data["Rows"].as<int>() : 9;
    tileWidth = data["TileWidth"] ? data["TileWidth"].as<int>() : 128;
    tileHeight = data["TileHeight"] ? data["TileHeight"].as<int>() : 128;

    if (serializedObjects && serializedObjects.IsSequence())
    {
        for (const YAML::Node &object : serializedObjects)
        {
            objects.push_back(YAML::Clone(object));
        }
    }
    return hasValidLayout();
}

bool TileMap::serialize(YAML::Emitter &out, const std::vector<GameObject> &objects) const
{
    out << YAML::BeginMap << YAML::Key << "TileMap" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Name" << YAML::Value << name;
    out << YAML::Key << "Columns" << YAML::Value << columns;
    out << YAML::Key << "Rows" << YAML::Value << rows;
    out << YAML::Key << "TileWidth" << YAML::Value << tileWidth;
    out << YAML::Key << "TileHeight" << YAML::Value << tileHeight;
    out << YAML::Key << "Objects" << YAML::Value << YAML::BeginSeq;
    for (const GameObject &object : objects)
    {
        GameObject serializable = object;
        YAML::Emitter objectEmitter;
        serializable.serialize(objectEmitter);
        YAML::Node saved = YAML::Load(objectEmitter.c_str());
        YAML::Node components = saved.begin()->second;
        if (components["EntityInfo"])
        {
            components["EntityInfo"].remove("UUID");
        }
        components.remove("ImportedGroup");
        components.remove("TileMapChild");
        out << saved;
    }
    out << YAML::EndSeq << YAML::EndMap << YAML::EndMap;
    return out.good();
}

bool TileMap::hasValidLayout() const
{
    return columns > 0 && rows > 0 && tileWidth > 0 && tileHeight > 0;
}

uint64_t TileMap::flattenIntoScene(const YAML::Node &asset, const TileMapImportContext &context)
{
    if (!context.createSceneObject)
    {
        return 0;
    }

    TileMap tileMap;
    std::vector<YAML::Node> objects;
    if (!tileMap.load(asset, objects))
    {
        return 0;
    }

    const uint64_t groupId = EntityInfo::generateUUID();
    for (const YAML::Node &serializedObject : objects)
    {
        if (!serializedObject || !serializedObject.IsMap() || serializedObject.size() != 1)
        {
            continue;
        }

        YAML::Node object = YAML::Clone(serializedObject);
        YAML::Node components = object.begin()->second;
        const Transform localTransform = transformFromComponents(components);
        components.remove("TileMapPlacement");
        components.remove("TileMapChild");
        components.remove("ImportedGroup");
        if (components["EntityInfo"])
        {
            components["EntityInfo"].remove("UUID");
        }
        writeTransform(components,
            transformFromModelMatrix(context.importTransform.getModelMatrix() * localTransform.getModelMatrix()));

        GameObject &sceneObject = context.createSceneObject(object);
        sceneObject.addComponent<ImportedGroup>();
        ImportedGroup &group = sceneObject.getComponent<ImportedGroup>();
        group.groupId = groupId;
        group.sourceAsset = context.sourceAsset;
    }
    return groupId;
}

Transform TileMap::transformFromModelMatrix(const glm::mat4 &modelMatrix)
{
    Transform transform;
    transform.position = glm::vec3(modelMatrix[3]);
    transform.scale.x = glm::length(glm::vec3(modelMatrix[0]));
    transform.scale.y = glm::length(glm::vec3(modelMatrix[1]));
    transform.scale.z = glm::length(glm::vec3(modelMatrix[2]));
    transform.rotation.z = glm::degrees(std::atan2(modelMatrix[0][1], modelMatrix[0][0]));
    return transform;
}
