#include "physics/Physics2D.h"
#include "physics/RigidBox2D.h"
#include "physics/BoxCollider2D.h"

#include "core/Transform.h"
#include "core/Scene.h"

#include <vector>
#include <box2d/box2d.h>
#include <memory>
#include <iostream>

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

Physics2D::Physics2D(Physics2D &&other) noexcept
    : m_worldId(other.m_worldId)
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
    float fixedDeltaTime = 0.0167f;
    b2World_Step(m_worldId, fixedDeltaTime, 4);
}

void Physics2D::setGravity(glm::vec2 gravity)
{
    b2World_SetGravity(m_worldId, b2Vec2{gravity.x, gravity.y});
}

// TODO: we should support other colliders than box collider
void Physics2D::addRigidbody(GameObject &obj)
{
    Transform &transform = obj.getComponent<Transform>();
    if (!obj.hasComponent<Rigidbody2D>())
    {
        obj.addComponent<Rigidbody2D>();
    }
    Rigidbody2D &rb = obj.getComponent<Rigidbody2D>();

    b2BodyId bodyId = createBodyHelper(obj);
    attachShapeHelper(bodyId, obj);
}

b2BodyId Physics2D::createBodyHelper(GameObject &obj)
{
    Transform &transform = obj.getComponent<Transform>();
    Rigidbody2D &rb = obj.getComponent<Rigidbody2D>();

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = (rb.m_type == BodyType::Static)    ? b2_staticBody
                   : (rb.m_type == BodyType::Dynamic) ? b2_dynamicBody
                                                      : b2_kinematicBody;
    // TODO: box2d uses the center of the object, and we have been using the bottom left
    // we need to synchronize them in a better way
    glm::vec2 gameObjectCenter = {
        transform.getPosition()->x + obj.getWidth() * 0.5,
        transform.getPosition()->y + obj.getHeight() * 0.5};

    bodyDef.position = {gameObjectCenter.x, gameObjectCenter.y};
    bodyDef.fixedRotation = rb.m_fixedRotation;

    b2BodyId bodyId = b2CreateBody(m_worldId, &bodyDef);
    if (!b2Body_IsValid(bodyId))
    {
        std::cerr << "[Physics2D] Failed to create body for GameObject: " << obj.getTag() << std::endl;
    }
    rb.m_runtimeBody = bodyId;

    return bodyId;
}

void Physics2D::attachShapeHelper(b2BodyId bodyId, GameObject &obj)
{
    // TODO: think this through. Added for testing physics
    if(!obj.hasComponent<BoxCollider2D>())
    {
        int width = obj.getWidth();
        int height = obj.getHeight();
        obj.addComponent<BoxCollider2D>(width, height);
    }

    BoxCollider2D &box = obj.getComponent<BoxCollider2D>();

    b2Polygon b2Shape = b2MakeBox(box.m_size.x * 0.5f, box.m_size.y * 0.5f);
    b2Transform shapeTransform = b2Transform_identity;
    shapeTransform.p = {box.m_offset.x, box.m_offset.y};

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = box.m_density;
    shapeDef.material.friction = box.m_friction;
    // shapeDef.material.restitution = box.m_restitution;
    b2ShapeId shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &b2Shape);

    if (!b2Shape_IsValid(shapeId))
    {
        std::cerr << "Failed to attach shape to body: " << obj.getTag() << std::endl;
    }

    box.m_runtimeFixture = shapeId;
}

void Physics2D::syncTransforms(const std::vector<GameObject> &gameObjects)
{
    for (auto &go : gameObjects)
    {
        if (!go.hasComponent<Rigidbody2D>())
        {
            continue;
        }

        auto &rb = go.getComponent<Rigidbody2D>();
        auto &transform = go.getComponent<Transform>();

        if (b2Body_IsValid(rb.m_runtimeBody))
        {
            b2Transform t = b2Body_GetTransform(rb.m_runtimeBody);
            transform.setPosition(t.p.x, t.p.y, 0);
            // transform.getRotation()->z = glm::degrees(t.q.angle);
        }
    }
}
