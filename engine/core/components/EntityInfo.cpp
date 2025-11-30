#include "core/components/EntityInfo.h"

#include <string>
#include <yaml-cpp/yaml.h>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <imgui.h>
#endif

#ifdef EDITOR_BUILD
void EntityInfo::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "EntityInfo";
    out << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "Tag" << YAML::Value << tag;
    out << YAML::Key << "Width" << YAML::Value << width;
    out << YAML::Key << "Height" << YAML::Value << height;
    out << YAML::EndMap;
}

void EntityInfo::deserialize(const YAML::Node &in)
{
    tag = in["EntityInfo"]["Tag"].as<std::string>();
    width = in["EntityInfo"]["Width"].as<unsigned int>();
    height = in["EntityInfo"]["Height"].as<unsigned int>();
}

void EntityInfo::drawInspector()
{
    ImGuiWrapper::Collapsable("Game Object",
        [&]
        {
            ImGuiWrapper::InputTextSimple("Tag", tag);

            ImGui::DragInt("Width", reinterpret_cast<int *>(&width), 1.0f, 0, INT_MAX);
            ImGui::DragInt("Height", reinterpret_cast<int *>(&height), 1.0f, 0, INT_MAX);
        });
}
#endif
