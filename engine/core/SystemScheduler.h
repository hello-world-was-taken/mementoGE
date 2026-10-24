#pragma once

#include "core/jobs/JobSystem.h"

#include <entt/entt.hpp>

class ISystem;
class Physics2D;
struct FrameContext;
struct SystemContext;

// Executes one whole system update and waits for it to finish. Main-thread
// systems stay on the caller; other systems use the owned worker pool.
class SystemScheduler
{
public:
    explicit SystemScheduler(JobSystemConfig config = {});

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
    JobSystem m_jobs;
};
