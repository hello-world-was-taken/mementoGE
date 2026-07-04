#pragma once

#include "core/jobs/JobSystem.h"

class ISystem;
struct SystemContext;

// Executes one whole system update and waits for it to finish. Main-thread
// systems stay on the caller; other systems use the owned worker pool.
class SystemScheduler
{
public:
    explicit SystemScheduler(JobSystemConfig config = {});

    void run(ISystem &system, SystemContext &context);

private:
    JobSystem m_jobs;
};
