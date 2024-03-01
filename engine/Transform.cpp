#include "Transform.h"

Transform::Transform()
{
    m_position = glm::vec3(0.0f, 0.0f, 0.0f);
    m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
}

Transform::~Transform()
{
}

void Transform::SetPosition(float x, float y, float z)
{
    m_position = glm::vec3(x, y, z);
}

void Transform::SetRotation(float x, float y, float z)
{
    m_rotation = glm::vec3(glm::degrees(x), glm::degrees(y), glm::degrees(z));
}

void Transform::SetScale(float x, float y, float z)
{
    m_scale = glm::vec3(x, y, z);
}

void Transform::Translate(float x, float y, float z)
{
    m_position += glm::vec3(x, y, z);
}

void Transform::Rotate(float x, float y, float z)
{
    m_rotation += glm::vec3(glm::degrees(x), glm::degrees(y), glm::degrees(z));
}

void Transform::Scale(float x, float y, float z)
{
    m_scale += glm::vec3(x, y, z);
}

glm::vec3 Transform::GetPosition()
{
    return m_position;
}

glm::vec3 Transform::GetRotation()
{
    return m_rotation;
}

glm::vec3 Transform::GetScale()
{
    return m_scale;
}