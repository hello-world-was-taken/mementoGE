#include "core/components/BoxCollider2D.h"

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

void BoxCollider2D::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "BoxCollider2D" << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq << size.x << size.y << YAML::EndSeq;
    out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq << offset.x << offset.y << YAML::EndSeq;

    out << YAML::Key << "density" << YAML::Value << density;
    out << YAML::Key << "friction" << YAML::Value << friction;
    out << YAML::Key << "restitution" << YAML::Value << restitution;

    out << YAML::EndMap;
}

void BoxCollider2D::deserialize(const YAML::Node &node)
{
    if (!node["BoxCollider2D"])
        return;

    const auto &data = node["BoxCollider2D"];

    size.x = data["size"][0].as<float>();
    size.y = data["size"][1].as<float>();

    offset.x = data["offset"][0].as<float>();
    offset.y = data["offset"][1].as<float>();

    density = data["density"].as<float>();
    friction = data["friction"].as<float>();
    restitution = data["restitution"].as<float>();
}

void BoxCollider2D::drawInspector()
{
    ImGuiWrapper::Collapsable("Box Collider 2D",
        [&]
        {
            SetFieldWidth();
            ImGui::DragFloat2("Size", &size.x, 0.01f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat2("Offset", &offset.x, 0.01f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Density", &density, 0.01f, 0.0f);

            SetFieldWidth();
            ImGui::DragFloat("Friction", &friction, 0.01f, 0.0f, 1.0f);

            SetFieldWidth();
            ImGui::DragFloat("Restitution", &restitution, 0.01f, 0.0f, 1.0f);
        });
}
#endif