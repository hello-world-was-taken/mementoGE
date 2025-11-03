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

    void simulate(float timestep, const std::vector<GameObject> &gameObjects); // Call each frame

    void addRigidbody(GameObject &obj);
    void removeRigidbody(entt::entity entity);

    void setGravity(glm::vec2 gravity);

private:
    void syncTransforms(const std::vector<GameObject> &gameObjects);
    void attachShape(b2BodyId bodyId, GameObject &obj);

    b2BodyId createBody(GameObject &obj);

private:
    b2WorldId m_worldId;
};