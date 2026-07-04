#pragma once

#include "core/SystemDescriptor.h"

#include <functional>
#include <map>
#include <string>
#include <vector>

// A scene's execution plan, keyed by descriptor names.
// Building validates semantic dependencies before orienting access conflicts.
class SystemGraph
{
public:
    // Names absent from this scene are optional dependencies. Descriptors must
    // stay constant while installed; adding a system rebuilds the scene graph.
    // Equally ready systems use registration order as a deterministic tie-break.
    // Invalid names, contradictory dependencies, and cycles throw invalid_argument.
    static SystemGraph build(const std::vector<SystemDescriptor> &descriptors);

    [[nodiscard]] const std::vector<std::string> &orderedSystems() const noexcept
    {
        return m_executionSequence;
    }

    [[nodiscard]] const std::map<std::string, std::vector<std::string>> &successors() const noexcept
    {
        return m_successors;
    }

private:
    // Borrows input descriptors only while build() runs.
    using SystemsByName = std::map<std::string, std::reference_wrapper<const SystemDescriptor>>;

    static SystemsByName validateAndIndexSystems(const std::vector<SystemDescriptor> &descriptors);
    static void validateTarget(const SystemDescriptor &descriptor, const std::string &target);
    static bool blank(const std::string &name);

    void addEdge(const std::string &from, const std::string &to);
    void addDependencyEdges(const std::vector<SystemDescriptor> &descriptors, const SystemsByName &byName);
    void sortTopologically(const std::vector<SystemDescriptor> &descriptors);
    std::string findCycle(const std::vector<SystemDescriptor> &descriptors) const;
    void addConflictEdges(const SystemsByName &byName);

    std::vector<std::string> m_executionSequence;
    std::map<std::string, std::vector<std::string>> m_successors;
};
