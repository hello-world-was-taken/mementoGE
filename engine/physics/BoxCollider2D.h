#pragma once

#include <glm/glm.hpp>
#include <box2d/box2d.h>

class BoxCollider2D
{
public:
    BoxCollider2D(float width, float height)
    {
        m_size = {width, height};
    }

    glm::vec2 m_size;
    glm::vec2 m_offset = {0.0f, 0.0f};

    float m_density = 1.0f;
    float m_friction = 0.3f;
    float m_restitution = 0.0f;

    b2ShapeId m_runtimeFixture = b2_nullShapeId;
};