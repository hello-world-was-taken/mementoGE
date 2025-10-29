#include "core/components/EnemyStats.h"

#ifdef EDITOR_BUILD
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

#ifdef EDITOR_BUILD
static const char *EnemyTypeNames[] = {"Minion", "Elite", "Boss"};
#endif

void EnemyStats::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "EnemyStats" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "type" << YAML::Value << static_cast<int>(type);
    out << YAML::Key << "health" << YAML::Value << health;
    out << YAML::Key << "moveSpeed" << YAML::Value << moveSpeed;
    out << YAML::Key << "attackPower" << YAML::Value << attackPower;

    out << YAML::Key << "attackCooldown" << YAML::Value << attackCooldown;
    out << YAML::Key << "detectionRadius" << YAML::Value << detectionRadius;
    out << YAML::Key << "attackRange" << YAML::Value << attackRange;

    out << YAML::Key << "moveLeftDistance" << YAML::Value << moveLeftDistance;
    out << YAML::Key << "moveRightDistance" << YAML::Value << moveRightDistance;

    out << YAML::EndMap;
}

void EnemyStats::deserialize(const YAML::Node &node)
{
    if (!node["EnemyStats"])
        return;

    const auto &data = node["EnemyStats"];

    type = static_cast<EnemyType>(data["type"].as<int>());
    health = data["health"].as<float>();
    moveSpeed = data["moveSpeed"].as<float>();
    attackPower = data["attackPower"].as<float>();

    attackCooldown = data["attackCooldown"].as<float>();
    detectionRadius = data["detectionRadius"].as<float>();
    attackRange = data["attackRange"].as<float>();

    moveLeftDistance = data["moveLeftDistance"].as<float>();
    moveRightDistance = data["moveRightDistance"].as<float>();
}

#ifdef EDITOR_BUILD
inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

void EnemyStats::drawInspector()
{
    ImGui::Separator();
    ImGui::Text("Enemy Stats");

    // Type Dropdown
    SetFieldWidth();
    int typeIndex = static_cast<int>(type);
    if (ImGui::Combo("Enemy Type", &typeIndex, EnemyTypeNames, IM_ARRAYSIZE(EnemyTypeNames)))
    {
        type = static_cast<EnemyType>(typeIndex);
    }

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

    SetFieldWidth();
    ImGui::DragFloat("Move Left Distance", &moveLeftDistance, 1.0f, 0.0f);

    SetFieldWidth();
    ImGui::DragFloat("Move Right Distance", &moveRightDistance, 1.0f, 0.0f);
}
#endif