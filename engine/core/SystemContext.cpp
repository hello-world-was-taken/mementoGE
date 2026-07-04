#include "core/SystemContext.h"

#include "core/components/BoxCollider2D.h"
#include "core/components/EntityInfo.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Transform.h"

SystemContext::SystemContext(entt::registry &registry, Physics2D &physics, const FrameContext &frame, bool editMode)
    : entities(registry), frame(frame), editMode(editMode), m_physics(physics)
{
}

void SystemContext::stepPhysics()
{
    entities.requireResourceWrite(SystemResource::PhysicsWorld);
    entities.requireRead<Sprite>();
    entities.requireRead<EntityInfo>();
    entities.requireWrite<Transform>();
    entities.requireWrite<RigidBody2D>();
    entities.requireWrite<BoxCollider2D>();
    entities.requireWrite<Sensor2D>();
    m_physics.simulate(frame.deltaTime, entities.m_registry, editMode);
}

void SystemContext::disablePhysicsBody(entt::entity entity)
{
    entities.requireResourceWrite(SystemResource::PhysicsWorld);
    const auto *body = entities.try_get<const RigidBody2D>(entity);
    if (body != nullptr && b2Body_IsValid(body->bodyId))
    {
        b2Body_Disable(body->bodyId);
    }
}
