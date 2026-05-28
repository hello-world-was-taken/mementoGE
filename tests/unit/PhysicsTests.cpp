#include <gtest/gtest.h>

#include "core/GameObject.h"
#include "core/Physics2D.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include <algorithm>
#include <entt/entt.hpp>
#include <limits>

using namespace ::testing;

namespace
{
struct RigidBodyCleanupObserver
{
    Physics2D *physics = nullptr;

    void onDestroy(entt::registry &registry, entt::entity entity)
    {
        physics->removeRigidbody(entity, registry);
    }
};

void expectPolygonNear(const b2Polygon &actual, const b2Polygon &expected)
{
    ASSERT_EQ(actual.count, expected.count);
    for (int i = 0; i < actual.count; ++i)
    {
        EXPECT_NEAR(actual.vertices[i].x, expected.vertices[i].x, 0.0001f);
        EXPECT_NEAR(actual.vertices[i].y, expected.vertices[i].y, 0.0001f);
    }
}
} // namespace

TEST(PhysicsTests, RemoveRigidbodyClearsSensorOverlaps)
{
    entt::registry registry;
    // zero gravity for deterministic behavior
    Physics2D physics({0.0f, 0.0f});

    // Create a regular rigid body entity
    GameObject bodyObj(registry);
    bodyObj.addComponent<RigidBody2D>();
    // register the body with the physics world
    physics.registerEntity(registry, bodyObj.getEntityId());

    auto bodyEntity = bodyObj.getEntityId();
    auto &rb = bodyObj.getComponent<RigidBody2D>();
    EXPECT_TRUE(b2Body_IsValid(rb.bodyId));

    // Create a sensor entity that will list the body as overlapping (but don't register
    // it with Box2D to avoid cross-shape interactions in this unit test)
    GameObject sensorObj(registry);
    sensorObj.addComponent<Sensor2D>();

    auto sensorEntity = sensorObj.getEntityId();
    auto &sensor = sensorObj.getComponent<Sensor2D>();

    // Simulate an overlap recorded by the physics system
    sensor.overlappingObjects.push_back(bodyEntity);
    ASSERT_EQ(sensor.overlappingObjects.size(), 1u);

    // Remove the rigid body and verify the sensor's list is cleared
    physics.removeRigidbody(bodyEntity, registry);

    // Body's Box2D id should now be set to the null id
    EXPECT_TRUE(B2_ID_EQUALS(rb.bodyId, b2_nullBodyId));
    // Sensor should no longer reference the removed entity
    EXPECT_TRUE(std::find(sensor.overlappingObjects.begin(), sensor.overlappingObjects.end(), bodyEntity) ==
                sensor.overlappingObjects.end());
}

TEST(PhysicsTests, DestroyingEntityCleansUpRigidBody)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});
    RigidBodyCleanupObserver observer{&physics};
    registry.on_destroy<RigidBody2D>().connect<&RigidBodyCleanupObserver::onDestroy>(observer);

    GameObject go(registry);
    go.addComponent<RigidBody2D>();
    physics.registerEntity(registry, go.getEntityId());

    b2BodyId bodyId = go.getComponent<RigidBody2D>().bodyId;
    ASSERT_TRUE(b2Body_IsValid(bodyId));

    go.getRegistry().destroy(go.getEntityId());

    EXPECT_FALSE(b2Body_IsValid(bodyId));
}

TEST(PhysicsTests, RebuildInitializesPhysicsComponentsFromRegistry)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});

    GameObject go(registry);
    go.addComponent<BoxCollider2D>();

    physics.rebuild(registry);

    ASSERT_TRUE(go.hasComponent<RigidBody2D>());
    EXPECT_TRUE(b2Body_IsValid(go.getComponent<RigidBody2D>().bodyId));
    EXPECT_TRUE(b2Shape_IsValid(go.getComponent<BoxCollider2D>().shapes.front().shapeId));
}

TEST(PhysicsTests, RegistersMultipleOffsetColliderShapes)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});
    GameObject go(registry);
    go.addComponent<RigidBody2D>();
    physics.registerEntity(registry, go.getEntityId());

    go.addComponent<BoxCollider2D>();
    auto &collider = go.getComponent<BoxCollider2D>();
    collider.shapes = {
        BoxColliderShape2D{"body", {10.0f, 6.0f}, {2.0f, -1.0f}},
        BoxColliderShape2D{"head", {10.0f, 4.0f}, {-3.0f, 5.0f}, 90.0f},
    };

    physics.registerEntity(registry, go.getEntityId());

    ASSERT_EQ(collider.shapes.size(), 2u);
    for (const auto &shape : collider.shapes)
    {
        ASSERT_TRUE(b2Shape_IsValid(shape.shapeId));
    }

    const b2Polygon body = b2Shape_GetPolygon(collider.shapes[0].shapeId);
    const b2Polygon head = b2Shape_GetPolygon(collider.shapes[1].shapeId);
    EXPECT_FLOAT_EQ(body.centroid.x, 2.0f);
    EXPECT_FLOAT_EQ(body.centroid.y, -1.0f);
    EXPECT_FLOAT_EQ(head.centroid.x, -3.0f);
    EXPECT_FLOAT_EQ(head.centroid.y, 5.0f);

    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();
    for (int i = 0; i < head.count; ++i)
    {
        minX = std::min(minX, head.vertices[i].x);
        maxX = std::max(maxX, head.vertices[i].x);
        minY = std::min(minY, head.vertices[i].y);
        maxY = std::max(maxY, head.vertices[i].y);
    }
    EXPECT_NEAR(maxX - minX, 4.0f, 0.0001f);
    EXPECT_NEAR(maxY - minY, 10.0f, 0.0001f);
}

TEST(PhysicsTests, SecondaryColliderShapeParticipatesInSensorOverlaps)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});

    GameObject target(registry);
    target.addComponent<RigidBody2D>();
    auto &targetBody = target.getComponent<RigidBody2D>();
    targetBody.type = BodyType::Dynamic;
    targetBody.velocity = {0.0f, 0.0f};
    physics.registerEntity(registry, target.getEntityId());

    target.addComponent<BoxCollider2D>();
    auto &collider = target.getComponent<BoxCollider2D>();
    collider.shapes = {
        BoxColliderShape2D{"primary", {2.0f, 2.0f}, {-10.0f, 0.0f}},
        BoxColliderShape2D{"secondary", {2.0f, 2.0f}, {10.0f, 0.0f}},
    };
    physics.registerEntity(registry, target.getEntityId());

    GameObject detector(registry);
    detector.addComponent<RigidBody2D>();
    physics.registerEntity(registry, detector.getEntityId());
    detector.addComponent<Sensor2D>();
    auto &sensor = detector.getComponent<Sensor2D>();
    auto &sensorShape = sensor.shapes.front();
    sensorShape.name = "detector";
    sensorShape.size = {2.0f, 2.0f};
    sensorShape.offset = {10.0f, 0.0f};
    sensor.resetAnimationState();
    physics.registerEntity(registry, detector.getEntityId());

    physics.simulate(1.0f / 60.0f, registry);

    EXPECT_NE(std::find(sensor.overlappingObjects.begin(), sensor.overlappingObjects.end(), target.getEntityId()),
        sensor.overlappingObjects.end());
}

TEST(PhysicsTests, SpriteFlipsMirrorColliderAndSensorShapes)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});
    GameObject go(registry);
    go.addComponent<Sprite>();
    auto &sprite = go.getComponent<Sprite>();
    go.addComponent<RigidBody2D>();
    physics.registerEntity(registry, go.getEntityId());

    go.addComponent<BoxCollider2D>();
    auto &colliderShape = go.getComponent<BoxCollider2D>().shapes.front();
    colliderShape.size = {10.0f, 4.0f};
    colliderShape.offset = {3.0f, 2.0f};
    colliderShape.rotation = 30.0f;
    physics.registerEntity(registry, go.getEntityId());

    go.addComponent<Sensor2D>();
    auto &sensorShape = go.getComponent<Sensor2D>().shapes.front();
    sensorShape.size = {6.0f, 2.0f};
    sensorShape.offset = {8.0f, 1.0f};
    physics.registerEntity(registry, go.getEntityId());

    sprite.flipX = true;
    physics.simulate(1.0f / 60.0f, registry);
    expectPolygonNear(b2Shape_GetPolygon(colliderShape.shapeId),
        b2MakeOffsetBox(5.0f, 2.0f, {-3.0f, 2.0f}, b2MakeRot(glm::radians(-30.0f))));
    expectPolygonNear(
        b2Shape_GetPolygon(sensorShape.shapeId), b2MakeOffsetBox(3.0f, 1.0f, {-8.0f, 1.0f}, b2Rot_identity));

    sprite.flipY = true;
    physics.simulate(1.0f / 60.0f, registry);
    expectPolygonNear(b2Shape_GetPolygon(colliderShape.shapeId),
        b2MakeOffsetBox(5.0f, 2.0f, {-3.0f, -2.0f}, b2MakeRot(glm::radians(30.0f))));
    expectPolygonNear(
        b2Shape_GetPolygon(sensorShape.shapeId), b2MakeOffsetBox(3.0f, 1.0f, {-8.0f, -1.0f}, b2Rot_identity));
}

TEST(PhysicsTests, ColliderUsesInitialSpriteFlipAsItsReferenceOrientation)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});
    GameObject go(registry);
    go.addComponent<Sprite>();
    auto &sprite = go.getComponent<Sprite>();
    sprite.flipX = true;
    go.addComponent<RigidBody2D>();
    physics.registerEntity(registry, go.getEntityId());

    go.addComponent<BoxCollider2D>();
    auto &collider = go.getComponent<BoxCollider2D>();
    auto &shape = collider.shapes.front();
    shape.size = {10.0f, 4.0f};
    shape.offset = {3.0f, 2.0f};
    shape.rotation = 30.0f;
    physics.registerEntity(registry, go.getEntityId());

    EXPECT_TRUE(collider.hasFlipReference);
    EXPECT_TRUE(collider.referenceFlipX);
    expectPolygonNear(
        b2Shape_GetPolygon(shape.shapeId), b2MakeOffsetBox(5.0f, 2.0f, {3.0f, 2.0f}, b2MakeRot(glm::radians(30.0f))));

    sprite.flipX = false;
    physics.simulate(1.0f / 60.0f, registry);
    expectPolygonNear(
        b2Shape_GetPolygon(shape.shapeId), b2MakeOffsetBox(5.0f, 2.0f, {-3.0f, 2.0f}, b2MakeRot(glm::radians(-30.0f))));
}

TEST(PhysicsTests, SynchronizesBodyRotationWithTransform)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});
    GameObject go(registry);
    auto &transform = go.getComponent<Transform>();
    transform.rotation.z = 25.0f;
    go.addComponent<RigidBody2D>();
    auto &body = go.getComponent<RigidBody2D>();
    body.type = BodyType::Dynamic;
    physics.registerEntity(registry, go.getEntityId());

    EXPECT_NEAR(glm::degrees(b2Rot_GetAngle(b2Body_GetRotation(body.bodyId))), 25.0f, 0.1f);

    b2Body_SetTransform(body.bodyId, b2Body_GetPosition(body.bodyId), b2MakeRot(glm::radians(-40.0f)));
    physics.simulate(1.0f / 60.0f, registry);

    EXPECT_NEAR(transform.rotation.z, -40.0f, 0.1f);
}

TEST(PhysicsTests, EditModeSynchronizesAuthoredTransformToPhysicsBody)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});
    GameObject go(registry);
    auto &transform = go.getComponent<Transform>();
    go.addComponent<RigidBody2D>();
    physics.registerEntity(registry, go.getEntityId());

    transform.position = {42.0f, -17.0f, transform.position.z};
    transform.rotation.z = 35.0f;
    physics.simulate(1.0f / 60.0f, registry, true);

    const auto &body = go.getComponent<RigidBody2D>();
    const b2Vec2 bodyPosition = b2Body_GetPosition(body.bodyId);
    EXPECT_NEAR(bodyPosition.x, 42.0f, 0.0001f);
    EXPECT_NEAR(bodyPosition.y, -17.0f, 0.0001f);
    EXPECT_NEAR(glm::degrees(b2Rot_GetAngle(b2Body_GetRotation(body.bodyId))), 35.0f, 0.1f);
    EXPECT_NEAR(transform.position.x, 42.0f, 0.0001f);
    EXPECT_NEAR(transform.position.y, -17.0f, 0.0001f);
}

TEST(PhysicsTests, SensorAnimationStateResetsToAuthoredValues)
{
    Sensor2D sensor;
    auto &shape = sensor.shapes.front();
    shape.name = "attack";
    shape.size = {12.0f, 8.0f};
    shape.offset = {3.0f, 1.0f};
    shape.isActive = false;
    shape.animatedSize = {20.0f, 20.0f};
    shape.animatedOffset = {-5.0f, 0.0f};
    shape.animatedActive = true;

    sensor.resetAnimationState();

    ASSERT_NE(sensor.findShape("attack"), nullptr);
    EXPECT_EQ(shape.animatedSize, shape.size);
    EXPECT_EQ(shape.animatedOffset, shape.offset);
    EXPECT_EQ(shape.animatedActive, shape.isActive);
}

TEST(PhysicsTests, ColliderAnimationStateResetsToAuthoredValues)
{
    BoxCollider2D collider;
    auto &shape = collider.shapes.front();
    shape.name = "head";
    shape.size = {12.0f, 8.0f};
    shape.offset = {3.0f, 1.0f};
    shape.rotation = 25.0f;
    shape.animatedSize = {20.0f, 20.0f};
    shape.animatedOffset = {-5.0f, 0.0f};
    shape.animatedRotation = -40.0f;
    shape.animatedActive = false;

    collider.resetAnimationState();

    ASSERT_NE(collider.findShape("head"), nullptr);
    EXPECT_EQ(shape.animatedSize, shape.size);
    EXPECT_EQ(shape.animatedOffset, shape.offset);
    EXPECT_EQ(shape.animatedRotation, shape.rotation);
    EXPECT_TRUE(shape.animatedActive);
}

TEST(PhysicsTests, AnimatedColliderShapeCanBeDisabledAndReenabled)
{
    entt::registry registry;
    Physics2D physics({0.0f, 0.0f});
    GameObject go(registry);
    go.addComponent<RigidBody2D>();
    physics.registerEntity(registry, go.getEntityId());

    go.addComponent<BoxCollider2D>();
    auto &shape = go.getComponent<BoxCollider2D>().shapes.front();
    shape.name = "handle";
    shape.size = {10.0f, 2.0f};
    physics.registerEntity(registry, go.getEntityId());
    ASSERT_TRUE(b2Shape_IsValid(shape.shapeId));

    shape.animatedActive = false;
    physics.simulate(1.0f / 60.0f, registry);
    EXPECT_FALSE(b2Shape_IsValid(shape.shapeId));

    shape.animatedSize = {14.0f, 4.0f};
    shape.animatedOffset = {2.0f, -1.0f};
    shape.animatedRotation = 35.0f;
    shape.animatedActive = true;
    physics.simulate(1.0f / 60.0f, registry);

    ASSERT_TRUE(b2Shape_IsValid(shape.shapeId));
    expectPolygonNear(
        b2Shape_GetPolygon(shape.shapeId), b2MakeOffsetBox(7.0f, 2.0f, {2.0f, -1.0f}, b2MakeRot(glm::radians(35.0f))));
}
