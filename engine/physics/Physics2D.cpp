#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Transform.h"
#include "core/components/EntityInfo.h"

#include "physics/Physics2D.h"

#include "core/Scene.h"

#include "util/Time.h"

#include <algorithm>
#include <box2d/box2d.h>
#include <entt/entt.hpp>
#include <iostream>
#include <memory>
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
    std::cout << "Physics2D destructor called: " << m_worldId.index1 << std::endl;
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

void Physics2D::simulate(float timestep, entt::registry &registry)
{
    auto view = registry.view<RigidBody2D>();
    for (const entt::entity &entity : view)
    {

        auto &rb = registry.get<RigidBody2D>(entity);
        b2Body_SetLinearVelocity(rb.bodyId, {rb.velocity.x, rb.velocity.y});
    }

    b2World_Step(m_worldId, Time::deltaTime(), 4);
    // process contact should be called after world step
    processContactEvents(registry);

    syncTransforms(registry);
}

void Physics2D::setGravity(glm::vec2 gravity)
{
    b2World_SetGravity(m_worldId, b2Vec2{gravity.x, gravity.y});
}

void Physics2D::registerRigidBody2D(GameObject &obj)
{
    createBody(obj);
}

void Physics2D::createBody(GameObject &obj)
{
    EntityInfo &entityInfo = obj.getComponent<EntityInfo>();
    Transform &transform = obj.getComponent<Transform>();
    RigidBody2D &rb = obj.getComponent<RigidBody2D>();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = (rb.type == BodyType::Static)    ? b2_staticBody
                   : (rb.type == BodyType::Dynamic) ? b2_dynamicBody
                                                    : b2_kinematicBody;

    bodyDef.position = {transform.position.x, transform.position.y};
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
    BoxCollider2D &box = obj.getComponent<BoxCollider2D>();

    b2Polygon b2Shape = b2MakeBox(box.size.x * 0.5f, box.size.y * 0.5f);
    b2Transform shapeTransform = b2Transform_identity;
    shapeTransform.p = {box.offset.x, box.offset.y};

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.enableSensorEvents = true;
    // store pointer to your entityId for later retrieval
    shapeDef.userData = reinterpret_cast<void *>(static_cast<uintptr_t>(obj.getEntityId()));
    shapeDef.density = box.density;
    shapeDef.material.friction = box.friction;
    b2ShapeId shapeId = b2CreatePolygonShape(rb.bodyId, &shapeDef, &b2Shape);

    if (!b2Shape_IsValid(shapeId))
    {
        std::cerr << "Failed to attach shape to body: " << entityInfo.tag << std::endl;
    }

    box.shapeId = shapeId;
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

    b2Polygon shape = b2MakeBox(sensor.size.x * 0.5f, sensor.size.y * 0.5f);
    b2ShapeDef shapeDef = b2DefaultShapeDef();

    shapeDef.isSensor = true;
    shapeDef.enableSensorEvents = true;
    // store pointer to your entityId for later retrieval
    shapeDef.userData = reinterpret_cast<void *>(static_cast<uintptr_t>(obj.getEntityId()));
    b2ShapeId shapeId = b2CreatePolygonShape(rb.bodyId, &shapeDef, &shape);

    if (!b2Shape_IsValid(shapeId))
    {
        std::cerr << "[Physics2D] Failed to create sensor shape for GameObject: " << entityInfo.tag << std::endl;
        return;
    }

    sensor.shapeId = shapeId;
}

void Physics2D::processContactEvents(entt::registry &registry)
{
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(m_worldId);

    // Begin sensors
    for (int i = 0; i < sensorEvents.beginCount; ++i)
    {
        const b2SensorBeginTouchEvent *e = sensorEvents.beginEvents + i;

        b2ShapeId sensorId = e->sensorShapeId;
        b2ShapeId vistorObjectId = e->visitorShapeId;

        entt::entity sensorEntity =
            static_cast<entt::entity>(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(sensorId)));
        entt::entity visitorEntity =
            static_cast<entt::entity>(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(vistorObjectId)));

        bool entityHasSensor2D = registry.all_of<Sensor2D>(sensorEntity);
        if (entityHasSensor2D)
        {
            auto &sensor = registry.get<Sensor2D>(sensorEntity);
            sensor.overlappingObjects.push_back(visitorEntity);
        }

        bool visitorHasSensor2D = registry.all_of<Sensor2D>(visitorEntity);
        if (visitorHasSensor2D)
        {
            auto &sensor = registry.get<Sensor2D>(visitorEntity);
            sensor.overlappingObjects.push_back(sensorEntity);
        }
    }

    // End contacts
    for (int i = 0; i < sensorEvents.endCount; ++i)
    {
        const b2SensorEndTouchEvent *e = sensorEvents.endEvents + i;

        b2ShapeId sensorId = e->sensorShapeId;
        b2ShapeId vistorObjectId = e->visitorShapeId;

        entt::entity sensorEntity =
            static_cast<entt::entity>(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(sensorId)));
        entt::entity visitorEntity =
            static_cast<entt::entity>(reinterpret_cast<uintptr_t>(b2Shape_GetUserData(vistorObjectId)));

        bool entityHasSensor2D = registry.all_of<Sensor2D>(sensorEntity);
        if (entityHasSensor2D)
        {
            auto &list = registry.get<Sensor2D>(sensorEntity).overlappingObjects;
            list.erase(std::remove(list.begin(), list.end(), visitorEntity), list.end());
        }

        bool visitorHasSensor2D = registry.all_of<Sensor2D>(visitorEntity);
        if (visitorHasSensor2D)
        {
            auto &list = registry.get<Sensor2D>(visitorEntity).overlappingObjects;
            list.erase(std::remove(list.begin(), list.end(), sensorEntity), list.end());

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
        }
    }
}
