#include "core/components/Patrol.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

#ifdef EDITOR_BUILD
void Patrol::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Patrol" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "moveLeftDistance" << YAML::Value << moveLeftDistance;
    out << YAML::Key << "moveRightDistance" << YAML::Value << moveRightDistance;
    out << YAML::Key << "direction" << YAML::Value << direction;

    out << YAML::EndMap;
}

void Patrol::deserialize(const YAML::Node &node)
{
    if (!node["Patrol"])
        return;

    const auto &data = node["Patrol"];

    moveLeftDistance = data["moveLeftDistance"].as<float>();
    moveRightDistance = data["moveRightDistance"].as<float>();
    direction = data["direction"].as<bool>();
}

inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

void Patrol::drawInspector()
{
    ImGuiWrapper::Collapsable("Patrol",
        [&]
        {
            SetFieldWidth();
            ImGui::DragFloat("Move Left Distance", &moveLeftDistance, 1.0f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Move Right Distance", &moveRightDistance, 1.0f, 0.0f);

            SetFieldWidth();
            ImGui::Checkbox("Direction", &direction);
        });
}
#endif