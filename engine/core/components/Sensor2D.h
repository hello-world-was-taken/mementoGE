#pragma once

#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <string>
#include <vector>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct SensorShape2D
{
    std::string name{"sensor"};
    glm::vec2 size{1.0f, 1.0f};
    glm::vec2 offset{0.0f, 0.0f};
    bool isActive = true;
    b2ShapeId shapeId = b2_nullShapeId;

    glm::vec2 animatedSize{1.0f, 1.0f};
    glm::vec2 animatedOffset{0.0f, 0.0f};
    bool animatedActive = true;

    // Runtime overlap results for this specific sensor shape. Keeping these on
    // the shape lets gameplay distinguish, for example, a foot sensor from an
    // attack sensor on the same entity.
    std::vector<entt::entity> overlappingObjects;

    void resetAnimationState();
};

struct Sensor2D
{
    bool isActive = true;
    std::vector<SensorShape2D> shapes{{}};

    // Store detected objects
    std::vector<entt::entity> overlappingObjects;

    void resetAnimationState();
    void clearOverlaps();
    SensorShape2D *findShape(const std::string &name);
    const SensorShape2D *findShape(const std::string &name) const;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);
    void drawInspector();
#endif
};
