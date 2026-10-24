#include "core/SystemScheduler.h"
#include "core/ISystem.h"
#include "core/SystemContext.h"

SystemScheduler::SystemScheduler(JobSystemConfig config)
    // Keep the waiting caller from taking worker updates out of the queue.
    : m_jobs(JobSystemConfig{config.workerCount, config.queueCapacity, false})
{
}

void SystemScheduler::run(ISystem &system,
    entt::registry &registry,
    Physics2D &physics,
    const FrameContext &frame,
    bool editMode)
{
    const auto descriptor = system.descriptor();
    const auto update = [&]
    {
        SystemContext context{registry, physics, frame, editMode};
        SystemEntities::UpdateScope access(context.entities, descriptor);
        system.update(context);
    };

    if (descriptor.threadAffinity() == SystemThreadAffinity::MainThread)
    {
        update();
        return;
    }

    m_jobs.runAndWait(update);
}

void SystemScheduler::run(ISystem &system, SystemContext &context)
{
    const auto descriptor = system.descriptor();
    const auto update = [&]
    {
        SystemEntities::UpdateScope access(context.entities, descriptor);
        system.update(context);
    };

    if (descriptor.threadAffinity() == SystemThreadAffinity::MainThread)
    {
        update();
        return;
    }

    m_jobs.runAndWait(update);
}
