#pragma once

#include <box2d/box2d.h>
#include <yaml-cpp/yaml.h>

// enum class BodyType
// {
//     Static = 0,
//     Dynamic = 1,
//     Kinematic = 2
// };

struct RigidBody2D
{
    b2BodyDef body;
    BodyType bodyType = BodyType::Dynamic;
    bool fixedRotation = false;

    void serialize(YAML::Emitter &out)
    {
        out << YAML::Key << "RigidBody2D";
        out << YAML::Value << YAML::BeginMap;
        out << YAML::Key << "BodyType" << YAML::Value << static_cast<int>(bodyType);
        out << YAML::Key << "FixedRotation" << YAML::Value << fixedRotation;
        out << YAML::EndMap;
    }

    void deserialize(const YAML::Node &node)
    {
        if (!node["RigidBody2D"])
            return;
        const auto &data = node["RigidBody2D"];
        bodyType = static_cast<BodyType>(data["BodyType"].as<int>());
        fixedRotation = data["FixedRotation"].as<bool>();
    }
};
