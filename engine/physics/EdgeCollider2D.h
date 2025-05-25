#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>

class EdgeCollider2D
{
public:
    glm::vec2 vertex1, vertex2;

    float friction = 0.3f;
    float restitution = 0.0f;

    b2ShapeId runtimeFixture = b2_nullShapeId;
};
