#pragma once

#include <box2d/box2d.h>
#include <glm/glm.hpp>
#include <string>

#ifdef EDITOR_BUILD
#include <yaml-cpp/yaml.h>
#endif

enum class BodyType
{
    Static = 0,
    Dynamic,
    Kinematic
};

struct RigidBody2D
{
    BodyType type = BodyType::Static;
    glm::vec2 velocity;
    bool fixedRotation = false;

    // owned and managed by PhysicsSystem
    b2BodyId bodyId = b2_nullBodyId;

#ifdef EDITOR_BUILD
    void serialize(YAML::Emitter &out);
    void deserialize(const YAML::Node &node);
    void drawInspector();
#endif
};