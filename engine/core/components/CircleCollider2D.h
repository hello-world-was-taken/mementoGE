#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

class CircleCollider2D
{
public:
    float radius = 0.5f;
    glm::vec2 offset = {0.0f, 0.0f};

    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;

    b2ShapeId shapeId = b2_nullShapeId;
};
