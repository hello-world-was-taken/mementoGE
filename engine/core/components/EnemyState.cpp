#include "core/components/EnemyState.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

#ifdef EDITOR_BUILD
static const char *EnemyTypeNames[] = {"Minion", "Elite", "Boss"};
static const char *AiStateNames[] = {"Idle", "Patrol", "Chase", "Fly", "Attack", "Return", "Dead"};
#endif

#ifdef EDITOR_BUILD
inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

void EnemyState::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "EnemyState" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "type" << YAML::Value << static_cast<int>(type);
    out << YAML::Key << "health" << YAML::Value << health;
    out << YAML::Key << "moveSpeed" << YAML::Value << moveSpeed;
    out << YAML::Key << "attackPower" << YAML::Value << attackPower;

    out << YAML::Key << "attackCooldown" << YAML::Value << attackCooldown;
    out << YAML::Key << "detectionRadius" << YAML::Value << detectionRadius;
    out << YAML::Key << "attackRange" << YAML::Value << attackRange;

    out << YAML::EndMap;
}

void EnemyState::deserialize(const YAML::Node &node)
{
    if (!node["EnemyState"])
        return;

    const auto &data = node["EnemyState"];

    type = static_cast<EnemyType>(data["type"].as<int>());
    health = data["health"].as<float>();
    moveSpeed = data["moveSpeed"].as<float>();
    attackPower = data["attackPower"].as<float>();

    attackCooldown = data["attackCooldown"].as<float>();
    detectionRadius = data["detectionRadius"].as<float>();
    attackRange = data["attackRange"].as<float>();
}

void EnemyState::drawInspector()
{
    ImGuiWrapper::Collapsable("Enemy Stats",
        [&]
        {
            SetFieldWidth();
            int typeIndex = static_cast<int>(type);
            if (ImGui::Combo("Enemy Type", &typeIndex, EnemyTypeNames, IM_ARRAYSIZE(EnemyTypeNames)))
            {
                type = static_cast<EnemyType>(typeIndex);
            }

            int aiStateIndex = static_cast<int>(state);
            ImGui::Text("AI State:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.8f, 1.0f, 1.0f), "%s", AiStateNames[aiStateIndex]);

            SetFieldWidth();
            ImGui::DragFloat("Health", &health, 1.0f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Move Speed", &moveSpeed, 0.1f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Attack Power", &attackPower, 0.1f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Attack Cooldown", &attackCooldown, 0.01f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Detection Radius", &detectionRadius, 1.0f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Attack Range", &attackRange, 1.0f, 0.0f);
        });
}
#endif