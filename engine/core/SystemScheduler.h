#pragma once

#include "core/SystemDescriptor.h"
#include "core/jobs/JobSystem.h"

#include <entt/entt.hpp>
#include <functional>
#include <map>
#include <string>
#include <vector>

class ISystem;
class Physics2D;
class SystemGraph;
struct FrameContext;
struct SystemContext;

// Runs independent whole-system updates in ready waves and waits for completion.
// Main-thread updates stay on the caller, outside worker batches.
class SystemScheduler
{
public:
    explicit SystemScheduler(JobSystemConfig config = {});

    // Missing systems are disabled graph nodes; their dependencies still apply.
    // Keep graph and enabled bindings fixed; instances, frame and services must
    // outlive the call. Owner access to registry/resources resumes after return.
    // Prepare all component pools on the registry owner before dispatch.
    // Returns successful update costs in milliseconds, excluding queue wait.
    // Zero workers and edit mode retain stable serial graph order.
    std::map<std::string, double> run(const SystemGraph &graph,
        const std::map<std::string, std::reference_wrapper<ISystem>> &enabledSystems,
        entt::registry &registry,
        Physics2D &physics,
        const FrameContext &frame,
        bool editMode = false);

    // Prepare the system's descriptor pools on the registry owner before
    // dispatch. Concurrent calls require independent instances/access and no
    // overlapping registry preparation or structural mutation.
    void run(ISystem &system,
        entt::registry &registry,
        Physics2D &physics,
        const FrameContext &frame,
        bool editMode = false);

    // Borrowed-context overload for standalone callers. Pools must already be
    // prepared, and the caller must not share this context between updates.
    void run(ISystem &system, SystemContext &context);

private:
    using SystemMap = std::map<std::string, std::reference_wrapper<ISystem>>;
    using DescriptorMap = std::map<std::string, SystemDescriptor>;
    using PredecessorCounts = std::map<std::string, std::size_t>;

    static DescriptorMap snapshotDescriptors(const SystemGraph &graph, const SystemMap &enabledSystems);
    static PredecessorCounts countPredecessors(const SystemGraph &graph);
    static std::vector<std::string> readyWave(const SystemGraph &graph, const PredecessorCounts &remaining);
    static void completeWave(
        const SystemGraph &graph, const std::vector<std::string> &wave, PredecessorCounts &remaining);

    static double invokeUpdate(ISystem &system,
        const SystemDescriptor &descriptor,
        entt::registry &registry,
        Physics2D &physics,
        const FrameContext &frame,
        bool editMode);
    double dispatchUpdate(ISystem &system,
        const SystemDescriptor &descriptor,
        entt::registry &registry,
        Physics2D &physics,
        const FrameContext &frame,
        bool editMode);

    JobSystem m_jobs;
};
