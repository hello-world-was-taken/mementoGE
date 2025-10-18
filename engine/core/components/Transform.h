#pragma once

#include <glm/glm.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <yaml-cpp/yaml.h>

// every game object has a transform
class Transform
{
private:
    // The position of the object on the screen
    glm::vec3 m_position;
    glm::vec3 m_scale;
    glm::vec3 m_rotation;
    glm::mat4x4 mModelMatrix = glm::mat4(1.0f);

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

    glm::vec3 *getPosition();
    glm::vec3 *getRotation();
    glm::vec3 *getScale();
    glm::mat4x4 getModelMatrix();

    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &in);
};