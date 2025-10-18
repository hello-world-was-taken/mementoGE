#pragma once

#include <box2d/box2d.h>
#include <string>
#include <yaml-cpp/yaml.h>
#include <glm/glm.hpp>

enum class BodyType
{
    Static = 0,
    Dynamic,
    Kinematic
};

class RigidBody2D
{
public:
    BodyType m_type = BodyType::Static;
    b2BodyId m_runtimeBody = b2_nullBodyId;
    glm::vec2 velocity;
    bool m_fixedRotation = false;

    void setType(BodyType type);
    void setVelocity(float velocityX, float velocityY);

    std::string getBodyType();
    glm::vec2 &getVelocity();

    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);
};