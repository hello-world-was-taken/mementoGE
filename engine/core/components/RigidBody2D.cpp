#include "core/components/RigidBody2D.h"

#include <string>

#ifdef EDITOR_BUILD
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

#ifdef EDITOR_BUILD
void RigidBody2D::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "RigidBody2D" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "BodyType" << YAML::Value << static_cast<int>(type);
    out << YAML::Key << "FixedRotation" << YAML::Value << fixedRotation;
    out << YAML::Key << "Velocity" << YAML::Value << YAML::Flow << YAML::BeginSeq << velocity.x << velocity.y << YAML::EndSeq;
    out << YAML::EndMap;
}

void RigidBody2D::deserialize(const YAML::Node &node)
{
    if (!node["RigidBody2D"])
        return;

    const auto &data = node["RigidBody2D"];
    type = static_cast<BodyType>(data["BodyType"].as<int>());
    fixedRotation = data["FixedRotation"].as<bool>();

    if (data["Velocity"])
    {
        velocity.x = data["Velocity"][0].as<float>();
        velocity.y = data["Velocity"][1].as<float>();
    }
}

void RigidBody2D::drawInspector()
{
    std::string typeString = (type == BodyType::Static)    ? "Static"
                       : (type == BodyType::Dynamic) ? "Dynamic"
                                                        : "Kinematic";
    ImGui::Separator();
    if (ImGui::BeginCombo("Rigidbody 2D Type", typeString.c_str()))
    {
        if (ImGui::Selectable("Static"))
            type = BodyType::Static;
        if (ImGui::Selectable("Dynamic"))
            type = BodyType::Dynamic;
        if (ImGui::Selectable("Kinematic"))
            type = BodyType::Kinematic;
        ImGui::EndCombo();
    }
}
#endif

