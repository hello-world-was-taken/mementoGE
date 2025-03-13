#include <iostream>
#include <yaml-cpp/yaml.h>

#include "core/Transform.h"

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

glm::vec3 *Transform::getPosition()
{
    return &m_position;
}

glm::vec3 *Transform::getRotation()
{
    return &m_rotation;
}

glm::vec3 *Transform::getScale()
{
    return &m_scale;
}

glm::mat4x4 Transform::getTransformMatrix()
{
    mTransformMatrix = glm::translate(mTransformMatrix, m_position);
    mTransformMatrix = glm::rotate(mTransformMatrix, glm::radians(m_rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
    mTransformMatrix = glm::rotate(mTransformMatrix, glm::radians(m_rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    mTransformMatrix = glm::rotate(mTransformMatrix, glm::radians(m_rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    mTransformMatrix = glm::scale(mTransformMatrix, m_scale);
    return mTransformMatrix;
}

void Transform::serialize(YAML::Emitter &out)
{
    out << YAML::Key << "Transform";
    out << YAML::Value << YAML::BeginMap;

    out << YAML::Key << "Position";
    out << YAML::Value << YAML::BeginSeq;
    out << m_position.x;
    out << m_position.y;
    out << m_position.z;
    out << YAML::EndSeq;

    out << YAML::Key << "Rotation";
    out << YAML::Value << YAML::BeginSeq;
    out << m_rotation.x;
    out << m_rotation.y;
    out << m_rotation.z;
    out << YAML::EndSeq;

    out << YAML::Key << "Scale";
    out << YAML::Value << YAML::BeginSeq;
    out << m_scale.x;
    out << m_scale.y;
    out << m_scale.z;
    out << YAML::EndSeq;

    out << YAML::EndMap;
}

void Transform::deserialize(const YAML::Node &in)
{
    m_position.x = in["Transform"]["Position"][0].as<float>();
    m_position.y = in["Transform"]["Position"][1].as<float>();
    m_position.z = in["Transform"]["Position"][2].as<float>();

    m_rotation.x = in["Transform"]["Rotation"][0].as<float>();
    m_rotation.y = in["Transform"]["Rotation"][1].as<float>();
    m_rotation.z = in["Transform"]["Rotation"][2].as<float>();

    m_scale.x = in["Transform"]["Scale"][0].as<float>();
    m_scale.y = in["Transform"]["Scale"][1].as<float>();
    m_scale.z = in["Transform"]["Scale"][2].as<float>();
}