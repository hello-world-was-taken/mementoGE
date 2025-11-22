#include "core/components/Sensor2D.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

#ifdef EDITOR_BUILD
inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

void Sensor2D::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Sensor2D" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq << size.x << size.y << YAML::EndSeq;
    out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq << offset.x << offset.y << YAML::EndSeq;
    out << YAML::Key << "isActive" << YAML::Value << isActive;

    out << YAML::EndMap;
}

void Sensor2D::deserialize(const YAML::Node &node)
{
    if (!node["Sensor2D"])
        return;

    const auto &data = node["Sensor2D"];

    size.x = data["size"][0].as<float>();
    size.y = data["size"][1].as<float>();

    offset.x = data["offset"][0].as<float>();
    offset.y = data["offset"][1].as<float>();

    isActive = data["isActive"].as<bool>();
}

void Sensor2D::drawInspector()
{
    ImGuiWrapper::Collapsable("Sensor 2D",
        [&]
        {
            SetFieldWidth();
            ImGui::DragFloat2("Size##sensor2d", &size.x, 0.01f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat2("Offset##sensor2d", &offset.x, 0.01f, 0.0f);

            SetFieldWidth();
            ImGui::Checkbox("Active##sensor2d", &isActive);
        });
}
#endif
