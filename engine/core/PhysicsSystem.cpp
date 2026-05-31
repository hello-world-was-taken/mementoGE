#include "core/PhysicsSystem.h"

#include "core/Physics2D.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Transform.h"

SystemDescriptor PhysicsSystem::descriptor() const
{
    return SystemDescriptor::named("PhysicsSystem")
        .reads<Sprite>()
        .writes<Transform, RigidBody2D, BoxCollider2D, Sensor2D>()
        .writesResource(SystemResource::PhysicsWorld)
        .after("AnimationSystem")
        .nonReentrant();
}

void PhysicsSystem::update(SystemContext &context)
{
    context.physics.simulate(context.deltaTime, context.registry, context.editMode);
}
