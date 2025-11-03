#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <vector>

class PolygonCollider2D
{
public:
    std::vector<glm::vec2> points; // game world space vertices
    glm::vec2 offset = {0.0f, 0.0f};

    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;

    b2ShapeId shapeId = b2_nullShapeId;
};
