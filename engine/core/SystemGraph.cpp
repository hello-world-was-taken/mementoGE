#include "core/SystemGraph.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <string>

SystemGraph SystemGraph::build(const std::vector<SystemDescriptor> &descriptors)
{
    SystemGraph graph;
    const auto byName = validateAndIndexSystems(descriptors);

    for (const auto &descriptor : descriptors)
    {
        graph.m_successors.try_emplace(descriptor.name());
    }

    graph.addDependencyEdges(descriptors, byName);
    graph.sortTopologically(descriptors);
    graph.addConflictEdges(byName);

    return graph;
}

bool SystemGraph::blank(const std::string &name)
{
    return std::all_of(name.begin(),
        name.end(),
        [](unsigned char character)
        {
            return std::isspace(character);
        });
}

SystemGraph::SystemsByName SystemGraph::validateAndIndexSystems(const std::vector<SystemDescriptor> &descriptors)
{
    SystemsByName byName;

    for (const auto &descriptor : descriptors)
    {
        const auto &name = descriptor.name();

        if (blank(name))
        {
            throw std::invalid_argument("System graph contains an empty system name");
        }

        if (!byName.emplace(name, std::cref(descriptor)).second)
        {
            throw std::invalid_argument("Duplicate system descriptor name '" + name + "'");
        }
    }

    return byName;
}

void SystemGraph::validateTarget(const SystemDescriptor &descriptor, const std::string &target)
{
    if (blank(target))
    {
        throw std::invalid_argument("System '" + descriptor.name() + "' has an empty dependency target");
    }

    if (target == descriptor.name())
    {
        throw std::invalid_argument("System '" + descriptor.name() + "' depends on itself");
    }
}

void SystemGraph::addEdge(const std::string &from, const std::string &to)
{
    auto &next = m_successors.at(from);

    if (std::find(next.begin(), next.end(), to) == next.end())
    {
        next.push_back(to);
    }
}

void SystemGraph::addDependencyEdges(const std::vector<SystemDescriptor> &descriptors, const SystemsByName &byName)
{
    for (const auto &descriptor : descriptors)
    {
        const auto &name = descriptor.name();

        for (const auto &target : descriptor.beforeSystems())
        {
            validateTarget(descriptor, target);
            const auto &after = descriptor.afterSystems();

            if (std::find(after.begin(), after.end(), target) != after.end())
            {
                throw std::invalid_argument(
                    "System '" + descriptor.name() + "' declares both before and after '" + target + "'");
            }

            const auto found = byName.find(target);

            if (found != byName.end())
            {
                addEdge(name, target);
            }
        }

        for (const auto &target : descriptor.afterSystems())
        {
            validateTarget(descriptor, target);

            const auto found = byName.find(target);

            if (found != byName.end())
            {
                addEdge(target, name);
            }
        }
    }
}

void SystemGraph::sortTopologically(const std::vector<SystemDescriptor> &descriptors)
{
    std::map<std::string, std::size_t> indegree;

    for (const auto &descriptor : descriptors)
    {
        indegree.emplace(descriptor.name(), 0);
    }

    for (const auto &[name, next] : m_successors)
    {
        for (const auto &dependent : next)
        {
            ++indegree.at(dependent);
        }
    }

    while (!indegree.empty())
    {
        const SystemDescriptor *ready = nullptr;

        // Scanning insertion order preserves ties independently of map ordering.
        for (const auto &descriptor : descriptors)
        {
            const auto pending = indegree.find(descriptor.name());

            if (pending != indegree.end() && pending->second == 0)
            {
                ready = &descriptor;
                break;
            }
        }

        if (ready == nullptr)
        {
            throw std::invalid_argument("System dependency cycle: " + findCycle(descriptors));
        }

        const auto &name = ready->name();
        m_executionSequence.push_back(name);
        indegree.erase(name);

        for (const auto &dependent : m_successors.at(name))
        {
            --indegree.at(dependent);
        }
    }
}

std::string SystemGraph::findCycle(const std::vector<SystemDescriptor> &descriptors) const
{
    // Find an actual cycle rather than reporting downstream blocked systems.
    std::map<std::string, int> state;

    for (const auto &descriptor : descriptors)
    {
        state.emplace(descriptor.name(), 0);
    }

    std::vector<std::string> path;
    std::string cycle;

    const std::function<bool(const std::string &)> visit = [&](const std::string &name)
    {
        state.at(name) = 1;
        path.push_back(name);

        for (const auto &next : m_successors.at(name))
        {
            if (state.at(next) == 1)
            {
                for (auto start = std::find(path.begin(), path.end(), next); start != path.end(); ++start)
                {
                    cycle += *start + " -> ";
                }

                cycle += next;
                return true;
            }

            if (state.at(next) == 0 && visit(next))
            {
                return true;
            }
        }

        path.pop_back();
        state.at(name) = 2;

        return false;
    };

    for (const auto &descriptor : descriptors)
    {
        const auto &name = descriptor.name();

        if (state.at(name) == 0 && visit(name))
        {
            break;
        }
    }

    return cycle;
}

void SystemGraph::addConflictEdges(const SystemsByName &byName)
{
    // Serialize access conflicts in the established dependency order so these
    // extra edges preserve direct and transitive semantic dependencies.
    for (std::size_t earlier = 0; earlier < m_executionSequence.size(); ++earlier)
    {
        for (std::size_t later = earlier + 1; later < m_executionSequence.size(); ++later)
        {
            const auto &from = m_executionSequence[earlier];
            const auto &to = m_executionSequence[later];

            if (byName.at(from).get().conflictsWith(byName.at(to).get()))
            {
                addEdge(from, to);
            }
        }
    }
}
