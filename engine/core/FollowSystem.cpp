#include "core/FollowSystem.h"

#include "core/components/Camera.h"
#include "core/components/EntityRef.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Transform.h"
#include "util/Time.h"

#include <algorithm>
#include <glm/glm.hpp>

void FollowSystem::onDisabled(entt::registry &registry)
{
    auto view = registry.view<EntityRef, RigidBody2D>();
    for (entt::entity entity : view)
        view.get<RigidBody2D>(entity).velocity = {0.0f, 0.0f};
}

void FollowSystem::update(entt::registry &registry)
{
    auto view = registry.view<EntityRef, Transform>();

    for (entt::entity entity : view)
    {
        EntityRef &entityRef = view.get<EntityRef>(entity);
        if (!entityRef.resolve(registry))
        {
            continue;
        }

        if (!registry.valid(entityRef.target) || !registry.any_of<Transform>(entityRef.target))
        {
            continue;
        }

        Transform &followerTransform = view.get<Transform>(entity);
        Transform &targetTransform = registry.get<Transform>(entityRef.target);

        if (registry.any_of<Camera>(entity))
        {
            Camera &camera = registry.get<Camera>(entity);
            const float zoom = std::max(camera.zoom, 0.01f);
            const glm::vec2 viewportSize{camera.logicalWidth * zoom, camera.logicalHeight * zoom};
            const glm::vec2 deadZoneSize{viewportSize.x * 0.45f, viewportSize.y * 0.45f};
            const glm::vec2 deadZoneInset = (viewportSize - deadZoneSize) * 0.5f;

            const float deadZoneLeft = followerTransform.position.x + deadZoneInset.x;
            const float deadZoneRight = deadZoneLeft + deadZoneSize.x;
            const float deadZoneBottom = followerTransform.position.y + deadZoneInset.y;
            const float deadZoneTop = deadZoneBottom + deadZoneSize.y;

            if (targetTransform.position.x < deadZoneLeft)
            {
                followerTransform.position.x = targetTransform.position.x - deadZoneInset.x;
            }
            else if (targetTransform.position.x > deadZoneRight)
            {
                followerTransform.position.x = targetTransform.position.x - deadZoneInset.x - deadZoneSize.x;
            }

            if (targetTransform.position.y < deadZoneBottom)
            {
                followerTransform.position.y = targetTransform.position.y - deadZoneInset.y;
            }
            else if (targetTransform.position.y > deadZoneTop)
            {
                followerTransform.position.y = targetTransform.position.y - deadZoneInset.y - deadZoneSize.y;
            }
            continue;
        }

        const bool followerHasRigidBody = registry.any_of<RigidBody2D>(entity);
        const bool targetHasRigidBody = registry.any_of<RigidBody2D>(entityRef.target);

        if (!followerHasRigidBody)
        {
            followerTransform = targetTransform;
            continue;
        }

        RigidBody2D &followerRigidBody = registry.get<RigidBody2D>(entity);

        if (targetHasRigidBody)
        {
            followerRigidBody.velocity = registry.get<RigidBody2D>(entityRef.target).velocity;
            continue;
        }

        const float deltaTime = Time::deltaTime();
        if (deltaTime <= 0.0f)
        {
            followerRigidBody.velocity = glm::vec2(0.0f, 0.0f);
            continue;
        }

        const glm::vec3 delta = targetTransform.position - followerTransform.position;
        followerRigidBody.velocity = glm::vec2(delta.x / deltaTime, delta.y / deltaTime);
    }
}
