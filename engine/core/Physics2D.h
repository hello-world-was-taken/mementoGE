#pragma once

#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <glm/glm.hpp>
#include <memory>

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

    void simulate(float timestep, entt::registry &registry, bool syncAuthoredTransforms = false);

    // Recreate physics resources for every physics entity in a fully loaded registry.
    void rebuild(entt::registry &registry);

    // Initialize physics resources for one entity added or changed at runtime.
    void registerEntity(entt::registry &registry, entt::entity entity);

    // Remove the physics body (and associated sensor contacts) for an entity.
    // The caller must pass the registry that owns the entity.
    void removeRigidbody(entt::entity entity, entt::registry &registry);

    void setGravity(glm::vec2 gravity);

private:
    void syncCollisionShapes(entt::registry &registry);
    void syncBodiesFromTransforms(entt::registry &registry, bool includeDynamicBodies);
    void refreshSensorOverlaps(entt::registry &registry);
    void syncTransforms(entt::registry &registry);

    void createBody(entt::registry &registry, entt::entity entity);
    void createBoxCollider(entt::registry &registry, entt::entity entity);
    void createSensor(entt::registry &registry, entt::entity entity);

private:
    b2WorldId m_worldId;
};
