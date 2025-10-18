#include "physics/RigidBody2D.h"

#include <box2d/box2d.h>
#include <string>
#include <yaml-cpp/yaml.h>

void RigidBody2D::setType(BodyType type)
{
    m_type = type;

    if (b2Body_IsValid(m_runtimeBody))
    {
        b2BodyType box2dType = (type == BodyType::Static)    ? b2_staticBody
                               : (type == BodyType::Dynamic) ? b2_dynamicBody
                                                             : b2_kinematicBody;

        b2Body_SetType(m_runtimeBody, box2dType);
    }
}

void RigidBody2D::setVelocity(float velocityX, float velocityY)
{
    velocity = {velocityX, velocityY};
    b2Body_SetLinearVelocity(m_runtimeBody, {velocityX, velocityY});
}

glm::vec2 &RigidBody2D::getVelocity()
{
    return velocity;
}

std::string RigidBody2D::getBodyType()
{
    return (m_type == BodyType::Static) ? "Static" : (m_type == BodyType::Dynamic) ? "Dynamic"
                                                                                   : "Kinematic";
}

void RigidBody2D::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "RigidBody2D";
    out << YAML::Value << YAML::BeginMap;
    out << YAML::Key << "BodyType" << YAML::Value << static_cast<int>(m_type);
    out << YAML::Key << "FixedRotation" << YAML::Value << m_fixedRotation;
    out << YAML::EndMap;
}

void RigidBody2D::deserialize(const YAML::Node &node)
{
    if (!node["RigidBody2D"])
        return;
    const auto &data = node["RigidBody2D"];
    m_type = static_cast<BodyType>(data["BodyType"].as<int>());
    m_fixedRotation = data["FixedRotation"].as<bool>();
}
