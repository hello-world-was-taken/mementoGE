#pragma once

#include "core/Physics2D.h"

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
    virtual int executionOrder() const
    {
        return 100;
    }
};
