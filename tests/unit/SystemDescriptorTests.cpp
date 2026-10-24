#include "engine/core/ISystem.h"
#include "engine/core/SystemDescriptor.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>

namespace
{
struct Position
{
};

struct Velocity
{
};

struct PreparedComponent
{
    PreparedComponent()
    {
        ++constructed;
    }

    inline static int constructed = 0;
    int value = 0;
};

class SystemWithoutDescriptor final : public ISystem
{
public:
    void update(SystemContext &) override
    {
    }
};

static_assert(std::is_abstract_v<SystemWithoutDescriptor>, "Systems must implement their own descriptor()");

template <typename T> bool contains(const std::vector<T> &values, const T &value)
{
    return std::find(values.begin(), values.end(), value) != values.end();
}
} // namespace

TEST(SystemDescriptorTests, BuilderRecordsAndDeduplicatesDeclaredAccess)
{
    const SystemDescriptor descriptor = SystemDescriptor::named("Movement")
                                            .reads<Position, const Position>()
                                            .writes<Velocity, Velocity>()
                                            .readsResource(SystemResource::Input)
                                            .readsResource(SystemResource::Input)
                                            .before("PhysicsSystem")
                                            .before("PhysicsSystem")
                                            .nonReentrant();

    EXPECT_EQ(descriptor.name(), "Movement");
    EXPECT_TRUE(descriptor.readsComponent<Position>());
    EXPECT_TRUE(descriptor.writesComponent<Velocity>());
    EXPECT_EQ(descriptor.componentReads().size(), 1u);
    EXPECT_EQ(descriptor.componentWrites().size(), 1u);
    EXPECT_EQ(descriptor.resourceReads().size(), 1u);
    EXPECT_EQ(descriptor.beforeSystems().size(), 1u);
    EXPECT_FALSE(descriptor.isReentrant());
}

TEST(SystemDescriptorTests, PreparingDeclaredPoolsNormalizesTypesWithoutCreatingEntitiesOrComponents)
{
    entt::registry registry;
    PreparedComponent::constructed = 0;
    const auto descriptor = SystemDescriptor::named("PreparedPools")
                                .reads<Position, const Position, const PreparedComponent &>()
                                .writes<PreparedComponent, const PreparedComponent>();
    EXPECT_EQ(std::as_const(registry).storage<Position>(), nullptr);
    EXPECT_EQ(std::as_const(registry).storage<PreparedComponent>(), nullptr);

    descriptor.prepareRegistry(registry);

    const auto *positions = std::as_const(registry).storage<Position>();
    const auto *components = std::as_const(registry).storage<PreparedComponent>();
    ASSERT_NE(positions, nullptr);
    ASSERT_NE(components, nullptr);
    EXPECT_EQ(positions->size(), 0u);
    EXPECT_EQ(components->size(), 0u);
    EXPECT_EQ(PreparedComponent::constructed, 0);
    auto entities = registry.view<entt::entity>();
    EXPECT_EQ(entities.begin(), entities.end());
    EXPECT_EQ(descriptor.componentReads().size(), 2u);
    EXPECT_EQ(descriptor.componentWrites().size(), 1u);

    descriptor.prepareRegistry(registry);

    EXPECT_EQ(std::as_const(registry).storage<Position>(), positions);
    EXPECT_EQ(std::as_const(registry).storage<PreparedComponent>(), components);
    EXPECT_EQ(PreparedComponent::constructed, 0);
    EXPECT_EQ(entities.begin(), entities.end());
}

TEST(SystemDescriptorTests, ReadOnlySystemsDoNotConflict)
{
    const SystemDescriptor first = SystemDescriptor::named("First").reads<Position>();
    const SystemDescriptor second = SystemDescriptor::named("Second").reads<Position>();

    EXPECT_FALSE(first.isReentrant());
    EXPECT_FALSE(first.conflictsWith(second));
    EXPECT_FALSE(second.conflictsWith(first));
}

TEST(SystemDescriptorTests, ComponentReadWriteConflictsAreSymmetric)
{
    const SystemDescriptor reader = SystemDescriptor::named("Reader").reads<Position>();
    const SystemDescriptor writer = SystemDescriptor::named("Writer").writes<Position>();

    EXPECT_TRUE(reader.conflictsWith(writer));
    EXPECT_TRUE(writer.conflictsWith(reader));
}

TEST(SystemDescriptorTests, ResourceReadWriteConflictsAreSymmetric)
{
    const SystemDescriptor reader = SystemDescriptor::named("Reader").readsResource(SystemResource::PhysicsWorld);
    const SystemDescriptor writer = SystemDescriptor::named("Writer").writesResource(SystemResource::PhysicsWorld);

    EXPECT_TRUE(reader.conflictsWith(writer));
    EXPECT_TRUE(writer.conflictsWith(reader));
}

TEST(SystemDescriptorTests, StructuralChangesConflictWithEveryComponentAccess)
{
    const SystemDescriptor structural = SystemDescriptor::named("Structural").requestsStructuralChanges();
    const SystemDescriptor unrelatedReader = SystemDescriptor::named("Reader").reads<Position>();
    const SystemDescriptor unrelatedWriter = SystemDescriptor::named("Writer").writes<Velocity>();

    EXPECT_TRUE(structural.hasStructuralChanges());
    EXPECT_TRUE(contains(structural.resourceWrites(), SystemResource::RegistryStructure));
    EXPECT_TRUE(structural.conflictsWith(unrelatedReader));
    EXPECT_TRUE(unrelatedReader.conflictsWith(structural));
    EXPECT_TRUE(structural.conflictsWith(unrelatedWriter));
    EXPECT_TRUE(unrelatedWriter.conflictsWith(structural));
}
