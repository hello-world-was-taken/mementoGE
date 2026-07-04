#include "core/SystemScheduler.h"
#include "core/ISystem.h"
#include "core/SystemContext.h"

SystemScheduler::SystemScheduler(JobSystemConfig config)
    // Keep the waiting caller from taking worker updates out of the queue.
    : m_jobs(JobSystemConfig{config.workerCount, config.queueCapacity, false})
{
}

void SystemScheduler::run(ISystem &system, SystemContext &context)
{
    const auto descriptor = system.descriptor();
    SystemEntities::UpdateScope access(context.entities, descriptor);

    if (descriptor.threadAffinity() == SystemThreadAffinity::MainThread)
    {
        system.update(context);
        return;
    }

    m_jobs.runAndWait(
        [&]
        {
            system.update(context);
        });
}
