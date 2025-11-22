#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

struct BoxCollider2D
{

    glm::vec2 size{1.0f, 1.0f};
    glm::vec2 offset{0.0f, 0.0f};

    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;

    b2ShapeId shapeId = b2_nullShapeId;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &out);
    void drawInspector();
#endif
};