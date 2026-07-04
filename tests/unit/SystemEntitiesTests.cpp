#include "core/ISystem.h"
#include "core/SystemEntities.h"
#include "core/SystemScheduler.h"
#include "core/components/BoxCollider2D.h"
#include "core/components/EntityInfo.h"
#include "core/components/RigidBody2D.h"
#include "core/components/Sensor2D.h"
#include "core/components/Sprite.h"
#include "core/components/Transform.h"

#include <cstddef>
#include <functional>
#include <optional>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace
{
struct Position
{
    int value = 0;
};

struct Velocity
{
    int value = 0;
};

class AccessSystem final : public ISystem
{
public:
    AccessSystem(SystemDescriptor descriptor, std::function<void(SystemContext &)> update)
        : m_descriptor(std::move(descriptor)), m_update(std::move(update))
    {
    }

    void update(SystemContext &context) override
    {
        m_update(context);
    }

    SystemDescriptor descriptor() const override
    {
        return m_descriptor;
    }

private:
    SystemDescriptor m_descriptor;
    std::function<void(SystemContext &)> m_update;
};

class SystemEntitiesModes : public ::testing::TestWithParam<std::size_t>
{
protected:
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    SystemContext context{registry, physics, frame};
};

using ReadView = decltype(std::declval<SystemEntities &>().view<const Position>());

template <typename Type, typename = void> struct ExposesStorage : std::false_type
{
};

template <typename Type>
struct ExposesStorage<Type, std::void_t<decltype(std::declval<Type &>().template storage<Position>())>>
    : std::true_type
{
};

static_assert(!std::is_convertible_v<SystemEntities &, entt::registry &>);
static_assert(!ExposesStorage<SystemEntities>::value);
static_assert(!ExposesStorage<ReadView>::value);

SystemDescriptor physicsAccess()
{
    return SystemDescriptor::named("SystemEntitiesTests.Physics")
        .reads<Sprite, EntityInfo>()
        .writes<Transform, RigidBody2D, BoxCollider2D, Sensor2D>()
        .writesResource(SystemResource::PhysicsWorld);
}
} // namespace

TEST_P(SystemEntitiesModes, MixedViewsPreserveConstnessAndOnlyVisitTheIntersection)
{
    const auto moving = registry.create();
    registry.emplace<Position>(moving, Position{5});
    registry.emplace<Velocity>(moving, Velocity{3});
    const auto stationary = registry.create();
    registry.emplace<Position>(stationary, Position{11});
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem system{SystemDescriptor::named("Movement").reads<Velocity>().writes<Position>(),
        [&](SystemContext &context)
        {
            auto view = context.entities.view<Position, const Velocity>();
            static_assert(std::is_same_v<decltype(view.get<Position>(moving)), Position &>);
            static_assert(std::is_same_v<decltype(view.get<const Velocity>(moving)), const Velocity &>);
            EXPECT_TRUE(view.contains(moving));
            EXPECT_FALSE(view.contains(stationary));
            int visited = 0;
            view.each([&](entt::entity entity, Position &position, const Velocity &velocity)
                {
                    EXPECT_EQ(entity, moving);
                    position.value += velocity.value;
                    ++visited;
                });
            EXPECT_EQ(visited, 1);
            EXPECT_EQ(context.entities.get<const Position>(moving).value, 8);
            EXPECT_EQ(context.entities.try_get<Position>(moving)->value, 8);

            const auto &readOnly = std::as_const(context.entities);
            static_assert(std::is_same_v<decltype(readOnly.get<Position>(moving)), const Position &>);
            static_assert(std::is_same_v<decltype(readOnly.try_get<Position>(moving)), const Position *>);
            auto readView = readOnly.view<Position, Velocity>();
            static_assert(std::is_same_v<decltype(readView.get<Position>(moving)), const Position &>);
            readView.each([](const Position &position, const Velocity &velocity)
                {
                    EXPECT_EQ(position.value, 8);
                    EXPECT_EQ(velocity.value, 3);
                });
        }};

    scheduler.run(system, context);

    EXPECT_EQ(registry.get<Position>(moving).value, 8);
    EXPECT_EQ(registry.get<Position>(stationary).value, 11);
    EXPECT_EQ(registry.get<Velocity>(moving).value, 3);
}

TEST_P(SystemEntitiesModes, UndeclaredReadsAndWritesAreRejectedBeforeMutation)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{7});
    registry.emplace<Velocity>(entity, Velocity{9});
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem system{SystemDescriptor::named("Reader").reads<Position>(),
        [&](SystemContext &context)
        {
            EXPECT_EQ(context.entities.get<const Position>(entity).value, 7);
            EXPECT_THROW((void)context.entities.get<Position>(entity), std::logic_error);
            EXPECT_THROW((void)context.entities.try_get<Position>(entity), std::logic_error);
            EXPECT_THROW((void)context.entities.view<Position>(), std::logic_error);
            EXPECT_THROW((void)context.entities.get<const Velocity>(entity), std::logic_error);
            EXPECT_THROW((void)context.entities.try_get<const Velocity>(entity), std::logic_error);
            EXPECT_THROW((void)context.entities.view<const Velocity>(), std::logic_error);
            EXPECT_THROW((void)(context.entities.all_of<Position, Velocity>(entity)), std::logic_error);
            // Presence checks must validate every requested type, even if an
            // earlier match could otherwise short-circuit the result.
            EXPECT_THROW((void)(context.entities.any_of<Position, Velocity>(entity)), std::logic_error);
        }};

    scheduler.run(system, context);

    EXPECT_EQ(registry.get<Position>(entity).value, 7);
    EXPECT_EQ(registry.get<Velocity>(entity).value, 9);
}

TEST_P(SystemEntitiesModes, ExclusiveUpdatesSafelyQueryInitiallyAbsentPools)
{
    const auto entity = registry.create();
    EXPECT_EQ(std::as_const(registry).storage<Position>(), nullptr);
    EXPECT_EQ(std::as_const(registry).storage<Velocity>(), nullptr);
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem system{SystemDescriptor::named("EmptyPools").reads<Position>().writes<Velocity>(),
        [&](SystemContext &context)
        {
            const auto &readOnly = std::as_const(context.entities);
            auto readView = readOnly.view<Position, Velocity>();
            EXPECT_EQ(readView.begin(), readView.end());
            EXPECT_FALSE(readView.contains(entity));
            EXPECT_EQ(readOnly.try_get<Position>(entity), nullptr);
            EXPECT_FALSE(readOnly.all_of<Position>(entity));
            EXPECT_FALSE((readOnly.any_of<Position, Velocity>(entity)));

            // Mutable EnTT views may create missing pools while this update
            // owns exclusive registry access. No component instances are added.
            auto writeView = context.entities.view<const Position, Velocity>();
            EXPECT_EQ(writeView.begin(), writeView.end());
            EXPECT_FALSE(writeView.contains(entity));
            EXPECT_EQ(context.entities.try_get<Velocity>(entity), nullptr);
        }};

    scheduler.run(system, context);

    EXPECT_TRUE(registry.valid(entity));
    EXPECT_FALSE((registry.any_of<Position, Velocity>(entity)));
}

TEST_P(SystemEntitiesModes, AccessAndSavedViewsExpireAfterSuccessAndFailure)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{4});
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    EXPECT_THROW((void)context.entities.valid(entity), std::logic_error);
    EXPECT_THROW((void)context.entities.get<const Position>(entity), std::logic_error);
    for (bool failUpdate : {false, true})
    {
        SCOPED_TRACE(failUpdate);
        std::optional<ReadView> saved;
        AccessSystem system{SystemDescriptor::named("ScopedReader").reads<Position>(),
            [&](SystemContext &context)
            {
                saved.emplace(context.entities.view<const Position>());
                EXPECT_EQ(*saved->begin(), entity);
                if (failUpdate)
                {
                    throw std::runtime_error("scoped update failed");
                }
            }};

        if (failUpdate)
        {
            EXPECT_THROW(scheduler.run(system, context), std::runtime_error);
        }
        else
        {
            EXPECT_NO_THROW(scheduler.run(system, context));
        }

        ASSERT_TRUE(saved.has_value());
        EXPECT_THROW((void)context.entities.get<const Position>(entity), std::logic_error);
        EXPECT_THROW((void)saved->contains(entity), std::logic_error);
        EXPECT_THROW((void)saved->get<const Position>(entity), std::logic_error);
        EXPECT_THROW(saved->each([](const Position &) {}), std::logic_error);
        EXPECT_THROW((void)saved->begin(), std::logic_error);
        EXPECT_THROW((void)saved->end(), std::logic_error);
    }
}

TEST_P(SystemEntitiesModes, ANewUpdateDoesNotReactivateAnOldView)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{12});
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    std::optional<ReadView> saved;
    AccessSystem first{SystemDescriptor::named("FirstReader").reads<Position>(),
        [&](SystemContext &context)
        {
            saved.emplace(context.entities.view<const Position>());
        }};
    scheduler.run(first, context);
    ASSERT_TRUE(saved.has_value());

    AccessSystem second{SystemDescriptor::named("SecondReader").reads<Position>(),
        [&](SystemContext &context)
        {
            EXPECT_THROW((void)saved->get<const Position>(entity), std::logic_error);
            EXPECT_EQ(context.entities.get<const Position>(entity).value, 12);
        }};
    scheduler.run(second, context);
}

TEST_P(SystemEntitiesModes, StructuralOperationsRequireTheStructuralDeclaration)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{7});
    const auto empty = registry.create();
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem system{SystemDescriptor::named("ComponentWriter").writes<Position>(),
        [&](SystemContext &context)
        {
            EXPECT_THROW((void)context.entities.create(), std::logic_error);
            EXPECT_THROW(context.entities.destroy(entity), std::logic_error);
            EXPECT_THROW((void)context.entities.emplace<Position>(empty), std::logic_error);
            EXPECT_THROW((void)context.entities.get_or_emplace<Position>(empty), std::logic_error);
            EXPECT_THROW(context.entities.remove<Position>(entity), std::logic_error);
        }};

    scheduler.run(system, context);

    EXPECT_TRUE(registry.valid(entity));
    EXPECT_EQ(registry.get<Position>(entity).value, 7);
    EXPECT_FALSE(registry.all_of<Position>(empty));
}

TEST_P(SystemEntitiesModes, StructuralDeclarationDoesNotGrantComponentWriteAccess)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{7});
    const auto empty = registry.create();
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem system{SystemDescriptor::named("StructuralReader").reads<Position>().requestsStructuralChanges(),
        [&](SystemContext &context)
        {
            EXPECT_THROW((void)context.entities.emplace<Position>(empty), std::logic_error);
            EXPECT_THROW((void)context.entities.get_or_emplace<Position>(empty), std::logic_error);
            EXPECT_THROW(context.entities.remove<Position>(entity), std::logic_error);
        }};

    scheduler.run(system, context);

    EXPECT_EQ(registry.get<Position>(entity).value, 7);
    EXPECT_FALSE(registry.all_of<Position>(empty));
}

TEST_P(SystemEntitiesModes, AuthorizedStructuralOperationsPreserveTheirResults)
{
    const auto entity = registry.create();
    registry.emplace<Position>(entity, Position{7});
    entt::entity created = entt::null;
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem system{SystemDescriptor::named("StructuralWriter").writes<Position>().requestsStructuralChanges(),
        [&](SystemContext &context)
        {
            created = context.entities.create();
            context.entities.emplace<Position>(created, Position{10});
            EXPECT_EQ(context.entities.get_or_emplace<Position>(created).value, 10);
            context.entities.remove<Position>(entity);
            EXPECT_FALSE(context.entities.all_of<Position>(entity));
            context.entities.destroy(created);
        }};

    scheduler.run(system, context);

    EXPECT_TRUE(registry.valid(entity));
    EXPECT_FALSE(registry.all_of<Position>(entity));
    EXPECT_FALSE(registry.valid(created));
}

TEST_P(SystemEntitiesModes, RemovingPhysicsComponentsOrEntitiesAlsoRequiresPhysicsWorldWriteAccess)
{
    const auto entity = registry.create();
    registry.emplace<RigidBody2D>(entity);
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem system{SystemDescriptor::named("StructuralWithoutPhysics")
                            .writes<RigidBody2D>()
                            .requestsStructuralChanges(),
        [&](SystemContext &context)
        {
            EXPECT_THROW(context.entities.destroy(entity), std::logic_error);
            EXPECT_THROW(context.entities.remove<RigidBody2D>(entity), std::logic_error);
        }};

    scheduler.run(system, context);

    EXPECT_TRUE(registry.valid(entity));
    EXPECT_TRUE(registry.all_of<RigidBody2D>(entity));

    AccessSystem allowed{SystemDescriptor::named("StructuralWithPhysics")
                             .writes<RigidBody2D>()
                             .requestsStructuralChanges()
                             .writesResource(SystemResource::PhysicsWorld),
        [&](SystemContext &context)
        {
            EXPECT_EQ(context.entities.remove<RigidBody2D>(entity), 1u);
        }};
    scheduler.run(allowed, context);
    EXPECT_TRUE(registry.valid(entity));
    EXPECT_FALSE(registry.all_of<RigidBody2D>(entity));
}

TEST_P(SystemEntitiesModes, PhysicsStepRequiresBothResourceAndCanonicalComponentAccess)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    frame.deltaTime = 1.0f / 60.0f;
    const auto checkDenied = [&](SystemDescriptor descriptor)
    {
        AccessSystem system{std::move(descriptor), [](SystemContext &context)
            {
                EXPECT_THROW(context.stepPhysics(), std::logic_error);
            }};
        scheduler.run(system, context);
    };
    checkDenied(SystemDescriptor::named("NoPhysicsResource")
                    .reads<Sprite, EntityInfo>()
                    .writes<Transform, RigidBody2D, BoxCollider2D, Sensor2D>());
    checkDenied(SystemDescriptor::named("ReadOnlyPhysicsResource")
                    .reads<Sprite, EntityInfo>()
                    .writes<Transform, RigidBody2D, BoxCollider2D, Sensor2D>()
                    .readsResource(SystemResource::PhysicsWorld));
    checkDenied(SystemDescriptor::named("MissingSpriteRead")
                    .reads<EntityInfo>()
                    .writes<Transform, RigidBody2D, BoxCollider2D, Sensor2D>()
                    .writesResource(SystemResource::PhysicsWorld));
    checkDenied(SystemDescriptor::named("MissingEntityInfoRead")
                    .reads<Sprite>()
                    .writes<Transform, RigidBody2D, BoxCollider2D, Sensor2D>()
                    .writesResource(SystemResource::PhysicsWorld));
    checkDenied(SystemDescriptor::named("MissingSensorWrite")
                    .reads<Sprite, EntityInfo>()
                    .writes<Transform, RigidBody2D, BoxCollider2D>()
                    .writesResource(SystemResource::PhysicsWorld));

    AccessSystem allowed{physicsAccess(), [](SystemContext &context)
        {
            EXPECT_NO_THROW(context.stepPhysics());
        }};
    EXPECT_NO_THROW(scheduler.run(allowed, context));
    EXPECT_THROW(context.stepPhysics(), std::logic_error);
}

TEST_P(SystemEntitiesModes, DisablingBodiesRequiresPhysicsWriteAndRigidBodyReadAccess)
{
    const auto entity = registry.create();
    registry.emplace<RigidBody2D>(entity);
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    AccessSystem missingResource{SystemDescriptor::named("BodyWithoutPhysics").reads<RigidBody2D>(),
        [&](SystemContext &context)
        {
            EXPECT_THROW(context.disablePhysicsBody(entity), std::logic_error);
        }};
    scheduler.run(missingResource, context);
    AccessSystem missingComponent{SystemDescriptor::named("PhysicsWithoutBody")
                                     .writesResource(SystemResource::PhysicsWorld),
        [&](SystemContext &context)
        {
            EXPECT_THROW(context.disablePhysicsBody(entity), std::logic_error);
        }};
    scheduler.run(missingComponent, context);

    AccessSystem allowed{SystemDescriptor::named("BodyControl")
                            .reads<RigidBody2D>()
                            .writesResource(SystemResource::PhysicsWorld),
        [&](SystemContext &context)
        {
            EXPECT_NO_THROW(context.disablePhysicsBody(entity));
        }};
    EXPECT_NO_THROW(scheduler.run(allowed, context));
    EXPECT_TRUE(registry.valid(entity));
}

INSTANTIATE_TEST_SUITE_P(WorkerCounts, SystemEntitiesModes, ::testing::Values(0u, 1u, 2u));
