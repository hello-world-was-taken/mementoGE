#pragma once

#include "core/FrameContext.h"
#include "core/Physics2D.h"
#include "core/SystemEntities.h"

#include <entt/entt.hpp>

// TODO: Compare a context per update with separating shared scene/frame state
// from independent per-update access state. Which is simpler to maintain?
struct SystemContext
{
    SystemContext(entt::registry &registry, Physics2D &physics, const FrameContext &frame, bool editMode = false);

    SystemEntities entities;
    const FrameContext &frame; // Immutable inputs; entity and physics operations are checked separately.
    bool editMode = false;

    void stepPhysics();
    void disablePhysicsBody(entt::entity entity);

private:
    Physics2D &m_physics;
};
