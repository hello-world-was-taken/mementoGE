#pragma once

#include "core/Camera.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"

#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>

class GameObject;

class Physics2D
{
public:
    Physics2D(const glm::vec2 &gravity);
    ~Physics2D();

    // Rule of 5
    Physics2D(const Physics2D &) = delete;
    Physics2D &operator=(const Physics2D &) = delete;

    Physics2D(Physics2D &&other) noexcept;
    Physics2D &operator=(Physics2D &&other) noexcept;

    void simulate(float timestep, entt::registry &registry);

    void registerRigidBody2D(GameObject &obj);
    void registerBoxCollider2D(GameObject &obj);
    void registerSensor2D(GameObject &obj);

    // Remove the physics body (and associated sensor contacts) for an entity.
    // The caller must pass the registry that owns the entity.
    void removeRigidbody(entt::entity entity, entt::registry &registry);

    void setGravity(glm::vec2 gravity);

private:
    void processContactEvents(entt::registry &registry);
    void syncTransforms(entt::registry &registry);

    void createBody(GameObject &obj);

private:
    b2WorldId m_worldId;
};