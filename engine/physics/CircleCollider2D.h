#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>

class CircleCollider2D
{
public:
    float radius = 0.5f;
    glm::vec2 offset = {0.0f, 0.0f};

    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;

    b2ShapeId runtimeFixture = b2_nullShapeId;
};
