#include "core/components/Sensor2D.h"

#ifdef EDITOR_BUILD
#include "core/ImGuiWrapper.h"
#include <cstddef>
#include <cstdio>
#include <imgui.h>
#include <yaml-cpp/yaml.h>
#endif

void SensorShape2D::resetAnimationState()
{
    animatedSize = size;
    animatedOffset = offset;
    animatedActive = isActive;
}

void Sensor2D::resetAnimationState()
{
    for (auto &shape : shapes)
    {
        shape.resetAnimationState();
    }
}

void Sensor2D::clearOverlaps()
{
    overlappingObjects.clear();
    for (auto &shape : shapes)
    {
        shape.overlappingObjects.clear();
    }
}

SensorShape2D *Sensor2D::findShape(const std::string &name)
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

const SensorShape2D *Sensor2D::findShape(const std::string &name) const
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

void Sensor2D::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Sensor2D" << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "isActive" << YAML::Value << isActive;
    out << YAML::Key << "shapes" << YAML::Value << YAML::BeginSeq;
    for (const auto &shape : shapes)
    {
        out << YAML::BeginMap;
        out << YAML::Key << "name" << YAML::Value << shape.name;
        out << YAML::Key << "size" << YAML::Value << YAML::Flow << YAML::BeginSeq << shape.size.x << shape.size.y
            << YAML::EndSeq;
        out << YAML::Key << "offset" << YAML::Value << YAML::Flow << YAML::BeginSeq << shape.offset.x << shape.offset.y
            << YAML::EndSeq;
        out << YAML::Key << "isActive" << YAML::Value << shape.isActive;
        out << YAML::EndMap;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
}

void Sensor2D::deserialize(const YAML::Node &node)
{
    if (!node["Sensor2D"])
    {
        return;
    }

    const auto &data = node["Sensor2D"];
    if (data["isActive"])
    {
        isActive = data["isActive"].as<bool>();
    }
    shapes.clear();

    auto readShape = [](const YAML::Node &shapeNode, const std::string &fallbackName)
    {
        SensorShape2D shape;
        shape.name = shapeNode["name"] ? shapeNode["name"].as<std::string>() : fallbackName;
        if (shapeNode["size"])
        {
            shape.size = {shapeNode["size"][0].as<float>(), shapeNode["size"][1].as<float>()};
        }
        if (shapeNode["offset"])
        {
            shape.offset = {shapeNode["offset"][0].as<float>(), shapeNode["offset"][1].as<float>()};
        }
        if (shapeNode["isActive"])
        {
            shape.isActive = shapeNode["isActive"].as<bool>();
        }
        shape.resetAnimationState();
        return shape;
    };

    if (data["shapes"])
    {
        for (std::size_t i = 0; i < data["shapes"].size(); ++i)
        {
            shapes.push_back(readShape(data["shapes"][i], "sensor_" + std::to_string(i)));
        }
    }
    else
    {
        shapes.push_back(readShape(data, "sensor"));
    }

    if (shapes.empty())
    {
        shapes.emplace_back();
    }
    resetAnimationState();
}

void Sensor2D::drawInspector()
{
    ImGuiWrapper::Collapsable("Sensor 2D",
        [&]
        {
            SetFieldWidth();
            ImGui::Checkbox("Active##sensor2d", &isActive);
            std::size_t shapeToRemove = shapes.size();
            for (std::size_t i = 0; i < shapes.size(); ++i)
            {
                auto &shape = shapes[i];
                ImGui::PushID(static_cast<int>(i));
                char name[64]{};
                std::snprintf(name, sizeof(name), "%s", shape.name.c_str());
                if (ImGui::InputText("Name", name, sizeof(name)))
                {
                    shape.name = name;
                }
                SetFieldWidth();
                ImGui::DragFloat2("Size", &shape.size.x, 0.01f, 0.01f);
                SetFieldWidth();
                ImGui::DragFloat2("Offset", &shape.offset.x, 0.01f);
                SetFieldWidth();
                ImGui::Checkbox("Active", &shape.isActive);
                shape.resetAnimationState();
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
            if (ImGui::Button("Add sensor shape"))
            {
                SensorShape2D shape;
                shape.name = "sensor_" + std::to_string(shapes.size());
                shape.resetAnimationState();
                shapes.push_back(shape);
            }
        });
}
#endif
