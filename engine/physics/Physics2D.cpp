#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Transform.h"

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
}

void Physics2D::setGravity(glm::vec2 gravity)
{
    b2World_SetGravity(m_worldId, b2Vec2{gravity.x, gravity.y});
}

// TODO: we should support other colliders than box collider
void Physics2D::addRigidbody(GameObject &obj)
{
    Transform &transform = obj.getComponent<Transform>();
    if (!obj.hasComponent<RigidBody2D>())
    {
        obj.addComponent<RigidBody2D>();
    }

    b2BodyId bodyId = createBodyHelper(obj);
    attachShapeHelper(bodyId, obj);
}

b2BodyId Physics2D::createBodyHelper(GameObject &obj)
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

    return bodyId;
}

void Physics2D::attachShapeHelper(b2BodyId bodyId, GameObject &obj)
{
    // TODO: think this through. Added for testing physics
    if (!obj.hasComponent<BoxCollider2D>())
    {
        int width = obj.getWidth();
        int height = obj.getHeight();
        obj.addComponent<BoxCollider2D>();
        obj.getComponent<BoxCollider2D>().size = {width, height};
    }

    BoxCollider2D &box = obj.getComponent<BoxCollider2D>();

    b2Polygon b2Shape = b2MakeBox(box.size.x * 0.5f, box.size.y * 0.5f);
    b2Transform shapeTransform = b2Transform_identity;
    shapeTransform.p = {box.offset.x, box.offset.y};

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = box.density;
    shapeDef.material.friction = box.friction;
    // shapeDef.material.restitution = box.m_restitution;
    b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &b2Shape);

    if (!b2Shape_IsValid(shapeId))
    {
        std::cerr << "Failed to attach shape to body: " << obj.getTag() << std::endl;
    }

    box.runtimeFixture = shapeId;
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
            // transform.getRotation()->z = glm::degrees(t.q.angle);
        }
    }
}
