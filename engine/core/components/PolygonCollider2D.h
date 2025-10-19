#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

class PolygonCollider2D
{
public:
    std::vector<glm::vec2> points; // game world space vertices
    glm::vec2 offset = {0.0f, 0.0f};

    float density = 1.0f;
    float friction = 0.3f;
    float restitution = 0.0f;

    b2ShapeId runtimeFixture = b2_nullShapeId;
};
