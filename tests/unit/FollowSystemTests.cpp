#include "engine/core/FollowSystem.h"
#include "engine/core/Physics2D.h"
#include "engine/core/components/Camera.h"
#include "engine/core/components/EntityInfo.h"
#include "engine/core/components/EntityRef.h"
#include "engine/core/components/RenderLayer.h"
#include "engine/core/components/Transform.h"

#include <entt/entt.hpp>
#include <gtest/gtest.h>

TEST(FollowSystemTests, ClampsCameraToBackgroundBounds)
{
    entt::registry registry;

    const entt::entity background = registry.create();
    EntityInfo &backgroundInfo = registry.emplace<EntityInfo>(background);
    backgroundInfo.width = 1536;
    backgroundInfo.height = 864;
    RenderLayer &backgroundLayer = registry.emplace<RenderLayer>(background);
    backgroundLayer.layer = RenderLayerType::Background;
    Transform &backgroundTransform = registry.emplace<Transform>(background);
    backgroundTransform.position = {256.0f, 144.0f, -50.0f};

    const entt::entity target = registry.create();
    Transform &targetTransform = registry.emplace<Transform>(target);

    const entt::entity cameraEntity = registry.create();
    Transform &cameraTransform = registry.emplace<Transform>(cameraEntity);
    registry.emplace<Camera>(cameraEntity);
    EntityRef &cameraTarget = registry.emplace<EntityRef>(cameraEntity);
    cameraTarget.target = target;

    FollowSystem system;
    Physics2D physics{{0.0f, 0.0f}};
    SystemContext context{registry, physics, 1.0f / 60.0f};

    targetTransform.position = {-1000.0f, -1000.0f, 0.0f};
    system.update(context);
    EXPECT_FLOAT_EQ(cameraTransform.position.x, -512.0f);
    EXPECT_FLOAT_EQ(cameraTransform.position.y, -288.0f);

    targetTransform.position = {2000.0f, 2000.0f, 0.0f};
    system.update(context);
    EXPECT_FLOAT_EQ(cameraTransform.position.x, 512.0f);
    EXPECT_FLOAT_EQ(cameraTransform.position.y, 288.0f);
}
