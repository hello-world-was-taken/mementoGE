#include "core/components/BoxCollider2D.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"

#include <cstddef>
#include <cstdio>
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

void BoxColliderShape2D::resetAnimationState()
{
    animatedSize = size;
    animatedOffset = offset;
    animatedRotation = rotation;
    animatedActive = true;
}

void BoxCollider2D::resetAnimationState()
{
    for (auto &shape : shapes)
    {
        shape.resetAnimationState();
    }
}

BoxColliderShape2D *BoxCollider2D::findShape(const std::string &name)
{
    for (auto &shape : shapes)
    {
        if (shape.name == name)
        {
            return &shape;
        }
    }
    return nullptr;
}

const BoxColliderShape2D *BoxCollider2D::findShape(const std::string &name) const
{
    for (const auto &shape : shapes)
    {
        if (shape.name == name)
        {
            return &shape;
        }
    }
    return nullptr;
}

#ifdef EDITOR_BUILD
inline void SetFieldWidth(float w = 120.0f)
{
    ImGui::SetNextItemWidth(w);
}

void BoxCollider2D::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "BoxCollider2D" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "referenceFlipX" << YAML::Value << referenceFlipX;
    out << YAML::Key << "referenceFlipY" << YAML::Value << referenceFlipY;
    out << YAML::Key << "shapes" << YAML::Value << YAML::BeginSeq;
    for (const auto &shape : shapes)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << shape.name;
        out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq << shape.size.x << shape.size.y
            << YAML::EndSeq;
        out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq << shape.offset.x << shape.offset.y
            << YAML::EndSeq;
        out << YAML::Key << "rotation" << YAML::Value << shape.rotation;
        out << YAML::Key << "density" << YAML::Value << shape.density;
        out << YAML::Key << "friction" << YAML::Value << shape.friction;
        out << YAML::Key << "restitution" << YAML::Value << shape.restitution;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

void BoxCollider2D::deserialize(const YAML::Node &node)
{
    if (!node["BoxCollider2D"])
    {
        return;
    }

    const auto &data = node["BoxCollider2D"];
    shapes.clear();
    hasFlipReference = data["referenceFlipX"] || data["referenceFlipY"];
    referenceFlipX = data["referenceFlipX"] ? data["referenceFlipX"].as<bool>() : false;
    referenceFlipY = data["referenceFlipY"] ? data["referenceFlipY"].as<bool>() : false;

    auto readShape = [](const YAML::Node &shapeNode, const std::string &fallbackName)
    {
        BoxColliderShape2D shape;
        shape.name = shapeNode["name"] ? shapeNode["name"].as<std::string>() : fallbackName;
        if (shapeNode["size"])
        {
            shape.size = {shapeNode["size"][0].as<float>(), shapeNode["size"][1].as<float>()};
        }
        if (shapeNode["offset"])
        {
            shape.offset = {shapeNode["offset"][0].as<float>(), shapeNode["offset"][1].as<float>()};
        }
        if (shapeNode["rotation"])
        {
            shape.rotation = shapeNode["rotation"].as<float>();
        }
        if (shapeNode["density"])
        {
            shape.density = shapeNode["density"].as<float>();
        }
        if (shapeNode["friction"])
        {
            shape.friction = shapeNode["friction"].as<float>();
        }
        if (shapeNode["restitution"])
        {
            shape.restitution = shapeNode["restitution"].as<float>();
        }
        shape.resetAnimationState();
        return shape;
    };

    if (data["shapes"])
    {
        for (std::size_t i = 0; i < data["shapes"].size(); ++i)
        {
            shapes.push_back(readShape(data["shapes"][i], "body_" + std::to_string(i)));
        }
    }
    else
    {
        shapes.push_back(readShape(data, "body"));
    }

    if (shapes.empty())
    {
        shapes.emplace_back();
    }
    resetAnimationState();
}

void BoxCollider2D::drawInspector()
{
    ImGuiWrapper::Collapsable("Box Collider 2D",
        [&]
        {
            std::size_t shapeToRemove = shapes.size();
            for (std::size_t i = 0; i < shapes.size(); ++i)
            {
                auto &shape = shapes[i];
                bool changed = false;
                ImGui::PushID(static_cast<int>(i));
                char name[64]{};
                std::snprintf(name, sizeof(name), "%s", shape.name.c_str());
                if (ImGui::InputText("Name", name, sizeof(name)))
                {
                    shape.name = name;
                    changed = true;
                }
                SetFieldWidth();
                changed |= ImGui::DragFloat2("Size", &shape.size.x, 0.01f, 0.01f);
                SetFieldWidth();
                changed |= ImGui::DragFloat2("Offset", &shape.offset.x, 0.01f);
                SetFieldWidth();
                changed |= ImGui::DragFloat("Rotation (Deg)", &shape.rotation, 1.0f);
                SetFieldWidth();
                ImGui::DragFloat("Density", &shape.density, 0.01f, 0.0f);
                SetFieldWidth();
                ImGui::DragFloat("Friction", &shape.friction, 0.01f, 0.0f, 1.0f);
                SetFieldWidth();
                ImGui::DragFloat("Restitution", &shape.restitution, 0.01f, 0.0f, 1.0f);
                if (changed)
                {
                    shape.resetAnimationState();
                }
                if (shapes.size() > 1 && ImGui::Button("Remove shape"))
                {
                    shapeToRemove = i;
                }
                ImGui::Separator();
                ImGui::PopID();
            }
            if (shapeToRemove < shapes.size())
            {
                if (b2Shape_IsValid(shapes[shapeToRemove].shapeId))
                {
                    b2DestroyShape(shapes[shapeToRemove].shapeId, true);
                }
                shapes.erase(shapes.begin() + static_cast<std::ptrdiff_t>(shapeToRemove));
            }
            if (ImGui::Button("Add collider shape"))
            {
                BoxColliderShape2D shape;
                shape.name = "body_" + std::to_string(shapes.size());
                shape.resetAnimationState();
                shapes.push_back(shape);
            }
        });
}
#endif
