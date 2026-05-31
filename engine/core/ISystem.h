#pragma once

#include "core/Physics2D.h"
#include "core/SystemDescriptor.h"

#include <entt/entt.hpp>

struct SystemContext
{
    entt::registry &registry;
    Physics2D &physics;
    float deltaTime;
    bool editMode = false;
};

// TODO: we can avoid the vtable lookup by directly
// passing the function ptrs. Would it be worth it in this case?
class ISystem
{
public:
    virtual ~ISystem() = default;
    virtual void update(SystemContext &context) = 0;
    virtual SystemDescriptor descriptor() const = 0;
    // TODO: Remove executionOrder() and its overrides once the scheduler enforces
    // descriptor before/after dependencies, preserves existing implicit ordering,
    // and deterministically orders otherwise unordered conflicting systems.
    virtual int executionOrder() const
    {
        return 100;
    }
};
