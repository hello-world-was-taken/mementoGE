#pragma once

#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <vector>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct Sensor2D
{
    glm::vec2 size{1.0f, 1.0f};
    glm::vec2 offset{0.0f, 0.0f};
    bool isActive = true;

    b2ShapeId shapeId = b2_nullShapeId;

    // Store detected objects
    std::vector<entt::entity> overlappingObjects;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);
    void drawInspector();
#endif
};
