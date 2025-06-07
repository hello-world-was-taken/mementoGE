#pragma once

#include "core/Transform.h"

#include "physics/RigidBox2D.h"
#include "physics/BoxCollider2D.h"

#include <memory>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <box2d/box2d.h>

class GameObject;

class Physics2D
{
public:
    // TODO: rule of 5
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

    void syncTransforms(const std::vector<GameObject> &gameObjects);
    void setGravity(glm::vec2 gravity);

private:
    b2BodyId createBodyHelper(GameObject &obj);
    void attachShapeHelper(b2BodyId bodyId, GameObject &obj);

    b2WorldId m_worldId;
};