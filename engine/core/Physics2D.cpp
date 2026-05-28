#include "core/components/BoxCollider2D.h"
#include "core/components/EntityInfo.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Transform.h"

#include "core/Physics2D.h"
#include "physics/CollisionShapeTransform.h"

#include "core/Scene.h"

#include "util/LogChannels.h"
#include "util/Time.h"

#include <algorithm>
#include <box2d/box2d.h>
#include <cmath>
#include <entt/entt.hpp>
#include <iostream>
#include <memory>
#include <sawyer/Logger.h>
#include <vector>

Physics2D::Physics2D(const glm::vec2 &gravity)
{
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = b2Vec2{gravity.x, gravity.y};
    m_worldId = b2CreateWorld(&worldDef);
}

Physics2D::~Physics2D()
{
    if (B2_IS_NON_NULL(m_worldId))
    {
        b2DestroyWorld(m_worldId);
    }
    LOG_DEBUG(LogChannel::Physics, "Physics2D destructor called for world {}", m_worldId.index1);
}

Physics2D::Physics2D(Physics2D &&other) noexcept : m_worldId(other.m_worldId)
{
    other.m_worldId = b2_nullWorldId;
}

Physics2D &Physics2D::operator=(Physics2D &&other) noexcept
{
    if (this != &other)
    {
        if (B2_IS_NON_NULL(m_worldId))
        {
            b2DestroyWorld(m_worldId);
        }

        m_worldId = other.m_worldId;
        other.m_worldId = b2_nullWorldId;
    }
    return *this;
}

void Physics2D::simulate(float timestep, entt::registry &registry, bool syncAuthoredTransforms)
{
    // Static and kinematic bodies may be moved by scene systems (for example,
    // editor transforms). Dynamic bodies remain physics-owned outside edit mode.
    syncBodiesFromTransforms(registry, syncAuthoredTransforms);

    syncCollisionShapes(registry);

    auto view = registry.view<RigidBody2D>();
    for (const entt::entity &entity : view)
    {

        auto &rb = registry.get<RigidBody2D>(entity);
        // Only push velocities to valid Box2D bodies. A bodyId can become
        // invalid if the underlying body was destroyed (for example, when
        // a game object with a RigidBody2D is removed from the world).
        if (b2Body_IsValid(rb.bodyId))
        {
            b2Body_SetLinearVelocity(rb.bodyId, {rb.velocity.x, rb.velocity.y});
        }
    }

    b2World_Step(m_worldId, timestep, 4);
    refreshSensorOverlaps(registry);

    syncTransforms(registry);
}

void Physics2D::syncBodiesFromTransforms(entt::registry &registry, bool includeDynamicBodies)
{
    constexpr float epsilon = 0.0001f;
    auto view = registry.view<RigidBody2D, Transform>();
    for (const entt::entity entity : view)
    {
        const auto &rb = view.get<RigidBody2D>(entity);
        if (!includeDynamicBodies && rb.type == BodyType::Dynamic)
        {
            continue;
        }
        const Transform &transform = view.get<Transform>(entity);
        if (!b2Body_IsValid(rb.bodyId))
        {
            continue;
        }

        const b2Vec2 bodyPosition = b2Body_GetPosition(rb.bodyId);
        const float bodyRotation = b2Rot_GetAngle(b2Body_GetRotation(rb.bodyId));
        const float authoredRotation = glm::radians(transform.rotation.z);
        if (std::abs(bodyPosition.x - transform.position.x) > epsilon ||
            std::abs(bodyPosition.y - transform.position.y) > epsilon ||
            std::abs(bodyRotation - authoredRotation) > epsilon)
        {
            b2Body_SetTransform(rb.bodyId, {transform.position.x, transform.position.y}, b2MakeRot(authoredRotation));
        }
    }
}

void Physics2D::setGravity(glm::vec2 gravity)
{
    b2World_SetGravity(m_worldId, b2Vec2{gravity.x, gravity.y});
}

void Physics2D::removeRigidbody(entt::entity entity, entt::registry &registry)
{
    if (!registry.valid(entity))
    {
        return;
    }

    if (registry.any_of<RigidBody2D>(entity))
    {
        auto &rb = registry.get<RigidBody2D>(entity);

        if (b2Body_IsValid(rb.bodyId))
        {
            b2DestroyBody(rb.bodyId);
        }

        rb.bodyId = b2_nullBodyId;
    }

    // Clear sensor overlap for this entity so no stale references remain.
    auto view = registry.view<Sensor2D>();
    for (entt::entity sensorEntity : view)
    {
        auto &list = view.get<Sensor2D>(sensorEntity).overlappingObjects;
        list.erase(std::remove(list.begin(), list.end(), entity), list.end());
    }
}

void Physics2D::registerRigidBody2D(GameObject &obj)
{
    createBody(obj);
}

void Physics2D::createBody(GameObject &obj)
{
    EntityInfo &entityInfo = obj.getComponent<EntityInfo>();
    const Transform &transform = obj.getComponent<Transform>();
    RigidBody2D &rb = obj.getComponent<RigidBody2D>();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = (rb.type == BodyType::Static)    ? b2_staticBody
                   : (rb.type == BodyType::Dynamic) ? b2_dynamicBody
                                                    : b2_kinematicBody;

    bodyDef.position = {transform.position.x, transform.position.y};
    bodyDef.rotation = b2MakeRot(glm::radians(transform.rotation.z));
    bodyDef.fixedRotation = rb.fixedRotation;

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);
    if (!b2Body_IsValid(bodyId))
    {
        std::cerr << "[Physics2D] Failed to create body for GameObject: " << entityInfo.tag << std::endl;
    }
    rb.bodyId = bodyId;
}

void Physics2D::registerBoxCollider2D(GameObject &obj)
{
    EntityInfo &entityInfo = obj.getComponent<EntityInfo>();
    if (!obj.hasComponent<BoxCollider2D>())
    {
        std::cerr << "GameObject " << entityInfo.tag << " missing BoxCollider2D component!\n";
        return;
    }

    if (!obj.hasComponent<RigidBody2D>())
    {
        obj.addComponent<RigidBody2D>();
        registerRigidBody2D(obj);
    }

    RigidBody2D &rb = obj.getComponent<RigidBody2D>();
    BoxCollider2D &collider = obj.getComponent<BoxCollider2D>();
    collider.resetAnimationState();
    const Sprite *sprite = obj.hasComponent<Sprite>() ? &obj.getComponent<Sprite>() : nullptr;
    if (sprite && !collider.hasFlipReference)
    {
        collider.referenceFlipX = sprite->flipX;
        collider.referenceFlipY = sprite->flipY;
        collider.hasFlipReference = true;
    }
    const bool flipX = sprite && sprite->flipX != collider.referenceFlipX;
    const bool flipY = sprite && sprite->flipY != collider.referenceFlipY;

    for (auto &box : collider.shapes)
    {
        if (!box.animatedActive)
        {
            continue;
        }
        const glm::vec2 offset = mirrorCollisionOffset(box.animatedOffset, flipX, flipY);
        const float rotation = mirrorCollisionRotation(box.animatedRotation, flipX, flipY);
        b2Polygon polygon = b2MakeOffsetBox(box.animatedSize.x * 0.5f,
            box.animatedSize.y * 0.5f,
            {offset.x, offset.y},
            b2MakeRot(glm::radians(rotation)));
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.enableSensorEvents = true;
        shapeDef.userData = reinterpret_cast<void *>(static_cast<uintptr_t>(obj.getEntityId()));
        shapeDef.density = box.density;
        shapeDef.material.friction = box.friction;
        shapeDef.material.restitution = box.restitution;
        box.shapeId = b2CreatePolygonShape(rb.bodyId, &shapeDef, &polygon);

        if (!b2Shape_IsValid(box.shapeId))
        {
            std::cerr << "Failed to attach shape '" << box.name << "' to body: " << entityInfo.tag << std::endl;
        }
    }
}

void Physics2D::registerSensor2D(GameObject &obj)
{
    EntityInfo &entityInfo = obj.getComponent<EntityInfo>();
    if (!obj.hasComponent<Sensor2D>())
    {
        std::cerr << "GameObject " << entityInfo.tag << " missing Sensor2D component!\n";
        return;
    }

    if (!obj.hasComponent<RigidBody2D>())
    {
        obj.addComponent<RigidBody2D>();
        registerRigidBody2D(obj);
    }

    RigidBody2D &rb = obj.getComponent<RigidBody2D>();
    Sensor2D &sensor = obj.getComponent<Sensor2D>();
    sensor.resetAnimationState();
    const Sprite *sprite = obj.hasComponent<Sprite>() ? &obj.getComponent<Sprite>() : nullptr;
    const bool flipX = sprite && sprite->flipX;
    const bool flipY = sprite && sprite->flipY;

    for (auto &sensorShape : sensor.shapes)
    {
        const glm::vec2 offset = mirrorCollisionOffset(sensorShape.animatedOffset, flipX, flipY);
        b2Polygon polygon = b2MakeOffsetBox(
            sensorShape.animatedSize.x * 0.5f, sensorShape.animatedSize.y * 0.5f, {offset.x, offset.y}, b2Rot_identity);
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.isSensor = true;
        shapeDef.enableSensorEvents = true;
        shapeDef.userData = reinterpret_cast<void *>(static_cast<uintptr_t>(obj.getEntityId()));
        sensorShape.shapeId = b2CreatePolygonShape(rb.bodyId, &shapeDef, &polygon);

        if (!b2Shape_IsValid(sensorShape.shapeId))
        {
            std::cerr << "[Physics2D] Failed to create sensor shape '" << sensorShape.name
                      << "' for GameObject: " << entityInfo.tag << std::endl;
        }
    }
}

void Physics2D::syncCollisionShapes(entt::registry &registry)
{
    auto colliders = registry.view<BoxCollider2D>();
    for (entt::entity entity : colliders)
    {
        auto &collider = colliders.get<BoxCollider2D>(entity);
        const Sprite *sprite = registry.try_get<Sprite>(entity);
        if (sprite && !collider.hasFlipReference)
        {
            collider.referenceFlipX = sprite->flipX;
            collider.referenceFlipY = sprite->flipY;
            collider.hasFlipReference = true;
        }
        const bool flipX = sprite && sprite->flipX != collider.referenceFlipX;
        const bool flipY = sprite && sprite->flipY != collider.referenceFlipY;
        for (auto &shape : collider.shapes)
        {
            if (!shape.animatedActive)
            {
                if (b2Shape_IsValid(shape.shapeId))
                {
                    b2DestroyShape(shape.shapeId, true);
                }
                shape.shapeId = b2_nullShapeId;
                continue;
            }
            const glm::vec2 offset = mirrorCollisionOffset(shape.animatedOffset, flipX, flipY);
            const float rotation = mirrorCollisionRotation(shape.animatedRotation, flipX, flipY);
            if (!b2Shape_IsValid(shape.shapeId))
            {
                if (!registry.all_of<RigidBody2D>(entity))
                {
                    continue;
                }
                const b2BodyId bodyId = registry.get<RigidBody2D>(entity).bodyId;
                if (!b2Body_IsValid(bodyId))
                {
                    continue;
                }
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.enableSensorEvents = true;
                shapeDef.userData = reinterpret_cast<void *>(static_cast<uintptr_t>(entity));
                shapeDef.density = shape.density;
                shapeDef.material.friction = shape.friction;
                shapeDef.material.restitution = shape.restitution;
                b2Polygon polygon = b2MakeOffsetBox(shape.animatedSize.x * 0.5f,
                    shape.animatedSize.y * 0.5f,
                    {offset.x, offset.y},
                    b2MakeRot(glm::radians(rotation)));
                shape.shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
                continue;
            }
            b2Polygon polygon = b2MakeOffsetBox(shape.animatedSize.x * 0.5f,
                shape.animatedSize.y * 0.5f,
                {offset.x, offset.y},
                b2MakeRot(glm::radians(rotation)));
            b2Shape_SetPolygon(shape.shapeId, &polygon);
            b2Shape_SetDensity(shape.shapeId, shape.density, true);
            b2Shape_SetFriction(shape.shapeId, shape.friction);
            b2Shape_SetRestitution(shape.shapeId, shape.restitution);
        }
    }

    auto sensors = registry.view<Sensor2D>();
    for (entt::entity entity : sensors)
    {
        const Sprite *sprite = registry.try_get<Sprite>(entity);
        const bool flipX = sprite && sprite->flipX;
        const bool flipY = sprite && sprite->flipY;
        for (auto &shape : sensors.get<Sensor2D>(entity).shapes)
        {
            const glm::vec2 offset = mirrorCollisionOffset(shape.animatedOffset, flipX, flipY);
            if (!b2Shape_IsValid(shape.shapeId))
            {
                if (!registry.all_of<RigidBody2D>(entity))
                {
                    continue;
                }
                const b2BodyId bodyId = registry.get<RigidBody2D>(entity).bodyId;
                if (!b2Body_IsValid(bodyId))
                {
                    continue;
                }
                b2ShapeDef shapeDef = b2DefaultShapeDef();
                shapeDef.isSensor = true;
                shapeDef.enableSensorEvents = true;
                shapeDef.userData = reinterpret_cast<void *>(static_cast<uintptr_t>(entity));
                b2Polygon polygon = b2MakeOffsetBox(
                    shape.animatedSize.x * 0.5f, shape.animatedSize.y * 0.5f, {offset.x, offset.y}, b2Rot_identity);
                shape.shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &polygon);
                continue;
            }
            b2Polygon polygon = b2MakeOffsetBox(
                shape.animatedSize.x * 0.5f, shape.animatedSize.y * 0.5f, {offset.x, offset.y}, b2Rot_identity);
            b2Shape_SetPolygon(shape.shapeId, &polygon);
        }
    }
}

void Physics2D::refreshSensorOverlaps(entt::registry &registry)
{
    auto view = registry.view<Sensor2D>();
    for (entt::entity sensorEntity : view)
    {
        auto &sensor = view.get<Sensor2D>(sensorEntity);
        sensor.overlappingObjects.clear();
        if (!sensor.isActive)
        {
            continue;
        }

        for (const auto &shape : sensor.shapes)
        {
            if (!shape.animatedActive || !b2Shape_IsValid(shape.shapeId))
            {
                continue;
            }

            const int capacity = b2Shape_GetSensorCapacity(shape.shapeId);
            if (capacity == 0)
            {
                continue;
            }

            std::vector<b2ShapeId> overlaps(static_cast<std::size_t>(capacity));
            const int count = b2Shape_GetSensorOverlaps(shape.shapeId, overlaps.data(), capacity);
            for (int i = 0; i < count; ++i)
            {
                if (!b2Shape_IsValid(overlaps[i]))
                {
                    continue;
                }
                entt::entity other =
                    static_cast<entt::entity>(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(overlaps[i])));
                if (registry.valid(other) && other != sensorEntity &&
                    std::find(sensor.overlappingObjects.begin(), sensor.overlappingObjects.end(), other) ==
                        sensor.overlappingObjects.end())
                {
                    sensor.overlappingObjects.push_back(other);
                }
            }
        }
    }
}

void Physics2D::syncTransforms(entt::registry &registry)
{
    auto view = registry.view<RigidBody2D, Transform>();
    for (const entt::entity &entity : view)
    {
        auto &rb = registry.get<RigidBody2D>(entity);
        auto &transform = registry.get<Transform>(entity);

        if (b2Body_IsValid(rb.bodyId))
        {
            b2Transform t = b2Body_GetTransform(rb.bodyId);
            transform.position = {t.p.x, t.p.y, transform.position.z};
            transform.rotation.z = glm::degrees(b2Rot_GetAngle(t.q));
        }
    }
}
