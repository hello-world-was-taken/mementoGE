#include "core/FollowSystem.h"

#include "core/components/Camera.h"
#include "core/components/EntityRef.h"
#include "core/components/EntityInfo.h"
#include "core/components/RenderLayer.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Transform.h"
#include "util/Time.h"

#include <algorithm>
#include <cmath>
#include <glm/glm.hpp>
#include <limits>
#include <optional>

namespace
{
struct WorldBounds
{
    float left;
    float right;
    float bottom;
    float top;
};

std::optional<WorldBounds> getBackgroundBounds(entt::registry &registry)
{
    auto backgrounds = registry.view<EntityInfo, RenderLayer, Transform>();
    WorldBounds bounds{std::numeric_limits<float>::max(),
        std::numeric_limits<float>::lowest(),
        std::numeric_limits<float>::max(),
        std::numeric_limits<float>::lowest()};
    bool foundBackground = false;

    for (entt::entity entity : backgrounds)
    {
        const RenderLayer &renderLayer = backgrounds.get<RenderLayer>(entity);
        if (renderLayer.layer != RenderLayerType::Background)
        {
            continue;
        }

        const EntityInfo &info = backgrounds.get<EntityInfo>(entity);
        const Transform &transform = backgrounds.get<Transform>(entity);
        const float halfWidth = static_cast<float>(info.width) * std::abs(transform.scale.x) * 0.5f;
        const float halfHeight = static_cast<float>(info.height) * std::abs(transform.scale.y) * 0.5f;

        bounds.left = std::min(bounds.left, transform.position.x - halfWidth);
        bounds.right = std::max(bounds.right, transform.position.x + halfWidth);
        bounds.bottom = std::min(bounds.bottom, transform.position.y - halfHeight);
        bounds.top = std::max(bounds.top, transform.position.y + halfHeight);
        foundBackground = true;
    }

    return foundBackground ? std::optional<WorldBounds>{bounds} : std::nullopt;
}

void clampCameraToBounds(Transform &cameraTransform, const glm::vec2 &viewportSize, const WorldBounds &bounds)
{
    const float maxCameraX = bounds.right - viewportSize.x;
    const float maxCameraY = bounds.top - viewportSize.y;

    cameraTransform.position.x = maxCameraX >= bounds.left
                                     ? std::clamp(cameraTransform.position.x, bounds.left, maxCameraX)
                                     : (bounds.left + bounds.right - viewportSize.x) * 0.5f;
    cameraTransform.position.y = maxCameraY >= bounds.bottom
                                     ? std::clamp(cameraTransform.position.y, bounds.bottom, maxCameraY)
                                     : (bounds.bottom + bounds.top - viewportSize.y) * 0.5f;
}
} // namespace

void FollowSystem::update(SystemContext &context)
{
    entt::registry &registry = context.registry;
    const std::optional<WorldBounds> backgroundBounds = getBackgroundBounds(registry);
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

            if (backgroundBounds)
            {
                clampCameraToBounds(followerTransform, viewportSize, *backgroundBounds);
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

        const float deltaTime = context.deltaTime;
        if (deltaTime <= 0.0f)
        {
            followerRigidBody.velocity = glm::vec2(0.0f, 0.0f);
            continue;
        }

        const glm::vec3 delta = targetTransform.position - followerTransform.position;
        followerRigidBody.velocity = glm::vec2(delta.x / deltaTime, delta.y / deltaTime);
    }
}
