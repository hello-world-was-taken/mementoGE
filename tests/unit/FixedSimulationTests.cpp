#include "engine/core/PhysicsSystem.h"
#include "engine/core/Scene.h"
#include "engine/core/SystemRegistry.h"
#include "engine/core/components/BoxCollider2D.h"
#include "engine/core/components/RigidBody2D.h"

#include <gtest/gtest.h>

namespace
{
struct PhysicsOutcome
{
    glm::vec3 position;
    glm::vec2 velocity;
};

PhysicsOutcome simulatePhysics(double elapsedTime)
{
    Scene scene{"fixed physics test"};
    scene.setGravity({0.0f, -10.0f});
    scene.addSystem("PhysicsSystem");
    scene.setAllSystemsEnabled(true);
    GameObject &body = scene.addGameObject(1, 1, "body");
    body.getComponent<Transform>().position = {0.0f, 10.0f, 0.0f};
    body.addComponent<RigidBody2D>();
    RigidBody2D &rigidBody = body.getComponent<RigidBody2D>();
    rigidBody.type = BodyType::Dynamic;
    rigidBody.velocity = {3.0f, 0.0f};
    body.addComponent<BoxCollider2D>();
    scene.getPhysics2d().registerEntity(body.getRegistry(), body.getEntityId());
    scene.play();

    FrameContext tick;
    tick.deltaTime = 1.0f / 60.0f;
    tick.elapsedTime = elapsedTime;
    for (int index = 0; index < 60; ++index)
    {
        scene.update(tick);
    }
    return {body.getComponent<Transform>().position, rigidBody.velocity};
}
} // namespace

TEST(FixedSimulationTests, ScenePhysicsUsesTickDeltaInsteadOfElapsedWallTime)
{
    SystemRegistry::instance().registerSystem<PhysicsSystem>("PhysicsSystem");
    const PhysicsOutcome baseline = simulatePhysics(0.0);
    const PhysicsOutcome laterWallTime = simulatePhysics(1000.0);

    EXPECT_FLOAT_EQ(laterWallTime.position.x, baseline.position.x);
    EXPECT_FLOAT_EQ(laterWallTime.position.y, baseline.position.y);
    EXPECT_FLOAT_EQ(laterWallTime.velocity.x, baseline.velocity.x);
    EXPECT_FLOAT_EQ(laterWallTime.velocity.y, baseline.velocity.y);
    EXPECT_NEAR(baseline.position.x, 3.0f, 0.001f);
    EXPECT_LT(baseline.position.y, 6.0f);
    EXPECT_NEAR(baseline.velocity.y, -10.0f, 0.001f);
}
