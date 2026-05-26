#include "core/PhysicsSystem.h"

#include "core/Physics2D.h"

void PhysicsSystem::update(SystemContext &context)
{
    context.physics.simulate(context.deltaTime, context.registry, context.editMode);
}
