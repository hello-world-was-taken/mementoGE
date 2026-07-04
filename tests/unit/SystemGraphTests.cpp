#include "engine/core/SystemGraph.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
struct Position
{
};

struct Velocity
{
};

bool hasEdge(const SystemGraph &graph, const std::string &source, const std::string &target)
{
    const auto &successors = graph.successors().at(source);
    return std::find(successors.begin(), successors.end(), target) != successors.end();
}

void expectInvalid(const std::vector<SystemDescriptor> &descriptors,
    const std::vector<std::string> &involvedNames)
{
    try
    {
        (void)SystemGraph::build(descriptors);
        FAIL() << "Expected the invalid dependency graph to be rejected";
    }
    catch (const std::invalid_argument &error)
    {
        for (const auto &name : involvedNames)
        {
            EXPECT_NE(std::string(error.what()).find(name), std::string::npos) << error.what();
        }
    }
}
} // namespace

TEST(SystemGraphTests, EmptyGraphHasNoWork)
{
    const auto graph = SystemGraph::build({});
    EXPECT_TRUE(graph.orderedSystems().empty());
    EXPECT_TRUE(graph.successors().empty());
}

TEST(SystemGraphTests, UnorderedSystemsFollowRegistrationOrderRatherThanMapOrder)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Zebra"),
        SystemDescriptor::named("Middle"),
        SystemDescriptor::named("Alpha"),
    });

    // Alphabetical map iteration must not change registration order.
    EXPECT_EQ(graph.orderedSystems(), (std::vector<std::string>{"Zebra", "Middle", "Alpha"}));
    EXPECT_TRUE(std::all_of(graph.successors().begin(), graph.successors().end(),
        [](const auto &entry) { return entry.second.empty(); }));
}

TEST(SystemGraphTests, ExplicitBeforeAndAfterOverrideRegistrationOrder)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Last").after("Middle"),
        SystemDescriptor::named("First").before("Middle"),
        SystemDescriptor::named("Middle"),
    });

    EXPECT_EQ(graph.orderedSystems(), (std::vector<std::string>{"First", "Middle", "Last"}));
    EXPECT_EQ(graph.successors().size(), 3u);
    EXPECT_EQ(graph.successors().at("First"), (std::vector<std::string>{"Middle"}));
    EXPECT_EQ(graph.successors().at("Middle"), (std::vector<std::string>{"Last"}));
    EXPECT_TRUE(graph.successors().at("Last").empty());
}

TEST(SystemGraphTests, NewlyReadySystemsUseRegistrationOrder)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Dependent").after("Producer"),
        SystemDescriptor::named("Producer"),
        SystemDescriptor::named("Unrelated"),
    });

    EXPECT_EQ(graph.orderedSystems(), (std::vector<std::string>{"Producer", "Dependent", "Unrelated"}));
}

TEST(SystemGraphTests, ComponentAndResourceReadOnlyAccessDoesNotAddEdges)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("First").reads<Position>().readsResource(SystemResource::PhysicsWorld),
        SystemDescriptor::named("Second").reads<Position>().readsResource(SystemResource::PhysicsWorld),
    });

    EXPECT_TRUE(graph.successors().at("First").empty());
    EXPECT_TRUE(graph.successors().at("Second").empty());
}

TEST(SystemGraphTests, ComponentWriteConflictsFollowRegistrationOrder)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Reader").reads<Position>(),
        SystemDescriptor::named("Writer").writes<Position>(),
        SystemDescriptor::named("OtherWriter").writes<Velocity>(),
    });

    EXPECT_EQ(graph.orderedSystems(), (std::vector<std::string>{"Reader", "Writer", "OtherWriter"}));
    EXPECT_TRUE(hasEdge(graph, "Reader", "Writer"));
    EXPECT_FALSE(hasEdge(graph, "Writer", "Reader"));
    EXPECT_TRUE(graph.successors().at("OtherWriter").empty());
}

TEST(SystemGraphTests, ResourceWritesConflictWithoutAliasingUnrelatedResources)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Reader").readsResource(SystemResource::AudioDevice),
        SystemDescriptor::named("Writer").writesResource(SystemResource::AudioDevice),
        SystemDescriptor::named("Unrelated").writesResource(SystemResource::AudioSettings),
    });

    EXPECT_TRUE(hasEdge(graph, "Reader", "Writer"));
    EXPECT_FALSE(hasEdge(graph, "Writer", "Reader"));
    EXPECT_TRUE(graph.successors().at("Unrelated").empty());
    EXPECT_FALSE(hasEdge(graph, "Reader", "Unrelated"));
    EXPECT_FALSE(hasEdge(graph, "Writer", "Unrelated"));
}

TEST(SystemGraphTests, StructuralMutationConflictsWithEveryComponentAccess)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Structure").requestsStructuralChanges(),
        SystemDescriptor::named("Reader").reads<Position>(),
        SystemDescriptor::named("Writer").writes<Velocity>(),
        SystemDescriptor::named("SecondStructure").requestsStructuralChanges(),
        SystemDescriptor::named("SnapshotOnly"),
    });

    EXPECT_TRUE(hasEdge(graph, "Structure", "Writer"));
    EXPECT_TRUE(hasEdge(graph, "Structure", "Reader"));
    EXPECT_TRUE(hasEdge(graph, "Structure", "SecondStructure"));
    EXPECT_FALSE(hasEdge(graph, "Structure", "SnapshotOnly"));
    EXPECT_FALSE(hasEdge(graph, "SecondStructure", "SnapshotOnly"));
}

TEST(SystemGraphTests, ConflictOrientationPreservesTransitiveSemanticDependencies)
{
    // Registration order has Last before First, but the explicit chain already
    // requires First before Last even though they do not name one another.
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Last").writes<Position>(),
        SystemDescriptor::named("Middle").before("Last"),
        SystemDescriptor::named("First").writes<Position>().before("Middle"),
    });

    EXPECT_EQ(graph.orderedSystems(), (std::vector<std::string>{"First", "Middle", "Last"}));
    EXPECT_TRUE(hasEdge(graph, "First", "Last"));
    EXPECT_FALSE(hasEdge(graph, "Last", "First"));
}

TEST(SystemGraphTests, MissingNamedTargetsAreOptional)
{
    const auto graph = SystemGraph::build({
        SystemDescriptor::named("Present").before("AbsentBefore").after("AbsentAfter"),
    });

    EXPECT_EQ(graph.orderedSystems(), (std::vector<std::string>{"Present"}));
    EXPECT_TRUE(graph.successors().at("Present").empty());
}

TEST(SystemGraphTests, RejectsEmptyNamesAndDependencyTargets)
{
    expectInvalid({SystemDescriptor::named("")}, {});
    expectInvalid({SystemDescriptor::named(" \t\n")}, {});
    expectInvalid({SystemDescriptor::named("BeforeEmpty").before("")}, {"BeforeEmpty"});
    expectInvalid({SystemDescriptor::named("AfterEmpty").after("")}, {"AfterEmpty"});
    expectInvalid({SystemDescriptor::named("BeforeBlank").before(" \t")}, {"BeforeBlank"});
    expectInvalid({SystemDescriptor::named("AfterBlank").after("\n")}, {"AfterBlank"});
}

TEST(SystemGraphTests, RejectsDuplicateDescriptorNames)
{
    expectInvalid({
                      SystemDescriptor::named("Duplicate"),
                      SystemDescriptor::named("Duplicate"),
                  },
        {"Duplicate"});
}

TEST(SystemGraphTests, RejectsSelfDependencies)
{
    expectInvalid({SystemDescriptor::named("SelfBefore").before("SelfBefore")}, {"SelfBefore"});
    expectInvalid({SystemDescriptor::named("SelfAfter").after("SelfAfter")}, {"SelfAfter"});
}

TEST(SystemGraphTests, RejectsContradictoryDeclarationsEvenWhenTargetIsAbsent)
{
    expectInvalid({SystemDescriptor::named("Contradiction").before("Absent").after("Absent")},
        {"Contradiction", "Absent"});
}

TEST(SystemGraphTests, RejectsCyclesAndNamesEverySystemInTheCycle)
{
    expectInvalid({
                      SystemDescriptor::named("First").before("Second"),
                      SystemDescriptor::named("Second").before("Third"),
                      SystemDescriptor::named("Third").before("First"),
                  },
        {"First", "Second", "Third"});
}
