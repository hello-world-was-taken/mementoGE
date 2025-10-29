#include "core/components/Transform.h"

#include <iostream>

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

glm::mat4x4 Transform::getModelMatrix()
{
    glm::mat4x4 modelMatrix = glm::mat4(1.0f);
    modelMatrix = glm::translate(modelMatrix, position);
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    modelMatrix = glm::rotate(modelMatrix, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    modelMatrix = glm::scale(modelMatrix, scale);
    return modelMatrix;
}

#ifdef EDITOR_BUILD

inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

void Transform::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Transform";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Position";
    out << YAML::Value << YAML::BeginSeq;
    out << position.x;
    out << position.y;
    out << position.z;
    out << YAML::EndSeq;

    out << YAML::Key << "Rotation";
    out << YAML::Value << YAML::BeginSeq;
    out << rotation.x;
    out << rotation.y;
    out << rotation.z;
    out << YAML::EndSeq;

    out << YAML::Key << "Scale";
    out << YAML::Value << YAML::BeginSeq;
    out << scale.x;
    out << scale.y;
    out << scale.z;
    out << YAML::EndSeq;

    out << YAML::EndMap;
}

void Transform::deserialize(const YAML::Node &in)
{
    position.x = in["Transform"]["Position"][0].as<float>();
    position.y = in["Transform"]["Position"][1].as<float>();
    position.z = in["Transform"]["Position"][2].as<float>();

    rotation.x = in["Transform"]["Rotation"][0].as<float>();
    rotation.y = in["Transform"]["Rotation"][1].as<float>();
    rotation.z = in["Transform"]["Rotation"][2].as<float>();

    scale.x = in["Transform"]["Scale"][0].as<float>();
    scale.y = in["Transform"]["Scale"][1].as<float>();
    scale.z = in["Transform"]["Scale"][2].as<float>();
}

void Transform::drawInspector()
{
    ImGuiWrapper::Collapsable("Transform",
        [&]
        {
            SetFieldWidth();
            ImGui::DragFloat3("Position", &position.x, 0.1f);

            ImGui::Spacing();

            SetFieldWidth();
            ImGui::DragFloat3("Rotation (Deg)", &rotation.x, 1.0f);

            ImGui::Spacing();

            SetFieldWidth();
            ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.0f, FLT_MAX);
        });
}
#endif