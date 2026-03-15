#include "core/components/EntityInfo.h"

#include <random>
#include <string>
#include <yaml-cpp/yaml.h>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <imgui.h>
#endif

// TODO: anyway we could use random function in ParticleSystem.cpp
// which itself should be moved to a util.
uint64_t EntityInfo::generateUUID()
{
    static std::random_device rd;
    static std::mt19937_64 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist(1);
    return dist(gen);
}

#ifdef EDITOR_BUILD
void EntityInfo::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "EntityInfo";
    out << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "UUID" << YAML::Value << uuid;
    out << YAML::Key << "Tag" << YAML::Value << tag;
    out << YAML::Key << "Width" << YAML::Value << width;
    out << YAML::Key << "Height" << YAML::Value << height;
    out << YAML::EndMap;
}

void EntityInfo::deserialize(const YAML::Node &in)
{
    if (in["EntityInfo"]["UUID"])
    {
        uuid = in["EntityInfo"]["UUID"].as<uint64_t>();
    }
    tag = in["EntityInfo"]["Tag"].as<std::string>();
    width = in["EntityInfo"]["Width"].as<unsigned int>();
    height = in["EntityInfo"]["Height"].as<unsigned int>();
}

void EntityInfo::drawInspector()
{
    ImGuiWrapper::Collapsable("Game Object",
        [&]
        {
            ImGui::Text("UUID: %llu", uuid);
            ImGuiWrapper::InputTextSimple("Tag", tag);

            ImGui::DragInt("Width", reinterpret_cast<int *>(&width), 1.0f, 0, INT_MAX);
            ImGui::DragInt("Height", reinterpret_cast<int *>(&height), 1.0f, 0, INT_MAX);
        });
}
#endif
