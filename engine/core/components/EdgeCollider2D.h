#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>

// TODO: add serialization
class EdgeCollider2D
{
public:
    glm::vec2 vertex1, vertex2;

    float friction = 0.3f;
    float restitution = 0.0f;

    b2ShapeId shapeId = b2_nullShapeId;
};
