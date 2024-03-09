#include <iostream>

#include "engine/core/Transform.h"

Transform::Transform(glm::vec3 position)
{
    m_position = position;
    m_scale = glm::vec3(1.0f, 1.0f, 1.0f);
    ;
    m_rotation = glm::vec3(0.0f, 0.0f, 0.0f);
    ;
}

Transform::Transform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale)
{
    m_position = position;
    m_scale = scale;
    m_rotation = rotation;
}

Transform::~Transform()
{
    // std::cout << "Transform destructor called" << std::endl;
}

void Transform::setPosition(float x, float y, float z)
{
    m_position = glm::vec3(x, y, z);
}

void Transform::setRotation(float x, float y, float z)
{
    m_rotation = glm::vec3(glm::degrees(x), glm::degrees(y), glm::degrees(z));
}

void Transform::setScale(float x, float y, float z)
{
    m_scale = glm::vec3(x, y, z);
}

void Transform::translate(float x, float y, float z)
{
    m_position += glm::vec3(x, y, z);
}

void Transform::rotate(float x, float y, float z)
{
    m_rotation += glm::vec3(glm::degrees(x), glm::degrees(y), glm::degrees(z));
}

void Transform::scale(float x, float y, float z)
{
    m_scale += glm::vec3(x, y, z);
}

glm::vec3 Transform::getPosition()
{
    return m_position;
}

glm::vec3 Transform::getRotation()
{
    return m_rotation;
}

glm::vec3 Transform::getScale()
{
    return m_scale;
}

glm::mat4x4 Transform::getTransformMatrix()
{
    glm::mat4x4 transformMatrix = glm::mat4(1.0f);
    transformMatrix = glm::translate(transformMatrix, m_position);
    transformMatrix = glm::rotate(transformMatrix, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    transformMatrix = glm::rotate(transformMatrix, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    transformMatrix = glm::rotate(transformMatrix, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    transformMatrix = glm::scale(transformMatrix, m_scale);
    return transformMatrix;
}