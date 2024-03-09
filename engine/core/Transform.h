#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>

class Transform
{
private:
    glm::vec3 m_position;
    glm::vec3 m_scale;
    glm::vec3 m_rotation;

public:
    Transform(glm::vec3 position);
    Transform(glm::vec3 position, glm::vec3 rotation, glm::vec3 scale);
    ~Transform();
    void setPosition(float x, float y, float z);
    void setRotation(float x, float y, float z);
    void setScale(float x, float y, float z);
    void translate(float x, float y, float z);
    void rotate(float x, float y, float z);
    void scale(float x, float y, float z);
    glm::vec3 getPosition();
    glm::vec3 getRotation();
    glm::vec3 getScale();
    glm::mat4x4 getTransformMatrix();
};