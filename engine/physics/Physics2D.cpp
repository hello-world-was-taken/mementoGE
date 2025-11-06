#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Transform.h"
#include "core/components/Sensor2D.h"

#include "physics/Physics2D.h"

#include "core/Scene.h"

#include "util/Time.h"

#include <box2d/box2d.h>
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

void Physics2D::simulate(float timestep, const std::vector<GameObject> &gameObjects)
{
    for (auto &go : gameObjects)
    {
        if (!go.hasComponent<RigidBody2D>())
        {
            continue;
        }

        auto &rb = go.getComponent<RigidBody2D>();
        b2Body_SetLinearVelocity(rb.bodyId, {rb.velocity.x, rb.velocity.y});
    }

    b2World_Step(m_worldId, Time::deltaTime(), 4);
    // process contact should be called after world step
    processContactEvents();

    syncTransforms(gameObjects);
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
        std::cerr << "[Physics2D] Failed to create body for GameObject: " << obj.getTag() << std::endl;
    }
    rb.bodyId = bodyId;
}

void Physics2D::registerBoxCollider2D(GameObject &obj)
{
    if (!obj.hasComponent<BoxCollider2D>())
    {
        std::cerr << "GameObject " << obj.getTag() << " missing BoxCollider2D component!\n";
        return;
    }

    // TODO: there might be cases where we just want sensors
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
    shapeDef.density = box.density;
    shapeDef.material.friction = box.friction;
    b2ShapeId shapeId = b2CreatePolygonShape(rb.bodyId, &shapeDef, &b2Shape);

    if (!b2Shape_IsValid(shapeId))
    {
        std::cerr << "Failed to attach shape to body: " << obj.getTag() << std::endl;
    }

    box.shapeId = shapeId;
}

void Physics2D::registerSensor2D(GameObject &obj)
{
    if (!obj.hasComponent<Sensor2D>())
    {
        std::cerr << "GameObject " << obj.getTag() << " missing Sensor2D component!\n";
        return;
    }

    // TODO: there might be cases where we just want sensors
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
    shapeDef.userData = &obj; // store pointer to your GameObject for later retrieval
    b2ShapeId shapeId = b2CreatePolygonShape(rb.bodyId, &shapeDef, &shape);

    if (!b2Shape_IsValid(shapeId))
    {
        std::cerr << "[Physics2D] Failed to create sensor shape for GameObject: " << obj.getTag() << std::endl;
        return;
    }

    sensor.shapeId = shapeId;
}

void Physics2D::processContactEvents()
{
    b2ContactEvents contactEvents = b2World_GetContactEvents(m_worldId);

    // Begin contacts
    for (int i = 0; i < contactEvents.beginCount; ++i)
    {
        const b2ContactBeginTouchEvent *e = contactEvents.beginEvents + i;

        b2ShapeId shapeA = e->shapeIdA;
        b2ShapeId shapeB = e->shapeIdB;

        GameObject *objA = static_cast<GameObject *>(b2Shape_GetUserData(shapeA));
        GameObject *objB = static_cast<GameObject *>(b2Shape_GetUserData(shapeB));

        if (!objA || !objB)
            continue;

        if (objA->hasComponent<Sensor2D>())
        {
            auto &sensor = objA->getComponent<Sensor2D>();
            sensor.overlappingObjects.push_back(objB);
        }
        if (objB->hasComponent<Sensor2D>())
        {
            auto &sensor = objB->getComponent<Sensor2D>();
            sensor.overlappingObjects.push_back(objA);
        }
    }

    // End contacts
    for (int i = 0; i < contactEvents.endCount; ++i)
    {
        const b2ContactEndTouchEvent *e = contactEvents.endEvents + i;

        b2ShapeId shapeA = e->shapeIdA;
        b2ShapeId shapeB = e->shapeIdB;

        GameObject *objA = static_cast<GameObject *>(b2Shape_GetUserData(shapeA));
        GameObject *objB = static_cast<GameObject *>(b2Shape_GetUserData(shapeB));

        if (!objA || !objB)
            continue;

        if (objA->hasComponent<Sensor2D>())
        {
            auto &list = objA->getComponent<Sensor2D>().overlappingObjects;
            list.erase(std::remove(list.begin(), list.end(), objB), list.end());
        }

        if (objB->hasComponent<Sensor2D>())
        {
            auto &list = objB->getComponent<Sensor2D>().overlappingObjects;
            list.erase(std::remove(list.begin(), list.end(), objA), list.end());
        }
    }
}

void Physics2D::syncTransforms(const std::vector<GameObject> &gameObjects)
{
    for (auto &go : gameObjects)
    {
        if (!go.hasComponent<RigidBody2D>())
        {
            continue;
        }

        auto &rb = go.getComponent<RigidBody2D>();
        auto &transform = go.getComponent<Transform>();

        if (b2Body_IsValid(rb.bodyId))
        {
            b2Transform t = b2Body_GetTransform(rb.bodyId);
            transform.position = {t.p.x, t.p.y, transform.position.z};
        }
    }
}
