#include "core/SystemScheduler.h"
#include "core/ISystem.h"
#include "core/SystemContext.h"
#include "core/SystemGraph.h"
#include "util/LogChannels.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <sawyer/Logger.h>
#include <stdexcept>
#include <utility>

SystemScheduler::SystemScheduler(JobSystemConfig config)
    // Keep the waiting caller from taking worker updates out of the queue.
    : m_jobs(JobSystemConfig{config.workerCount, config.queueCapacity, false})
{
}

std::map<std::string, double> SystemScheduler::run(const SystemGraph &graph,
    const SystemMap &enabledSystems,
    entt::registry &registry,
    Physics2D &physics,
    const FrameContext &frame,
    bool editMode)
{
    const auto descriptors = snapshotDescriptors(graph, enabledSystems);
    std::map<std::string, double> timings;
    for (const auto &[name, system] : enabledSystems)
    {
        timings.emplace(name, 0.0);
    }

    if (m_jobs.workerCount() == 0 || editMode)
    {
        LOG_DEBUG(LogChannel::SystemScheduler,
            "Serial tick: {} enabled systems, concurrency limit {}, {} workers ({})",
            enabledSystems.size(),
            enabledSystems.empty() ? 0 : 1,
            m_jobs.workerCount(),
            editMode ? "edit mode" : "zero-worker mode");
        for (const auto &name : graph.orderedSystems())
        {
            const auto enabled = enabledSystems.find(name);
            if (enabled != enabledSystems.end())
            {
                timings.at(name) =
                    dispatchUpdate(enabled->second.get(), descriptors.at(name), registry, physics, frame, editMode);
            }
        }
        return timings;
    }

    const bool logConcurrency = Logger::shouldLog(LogLevel::Debug, LogChannel::SystemScheduler);
    auto remaining = countPredecessors(graph);
    std::size_t waveNumber = 0;
    while (!remaining.empty())
    {
        const auto wave = readyWave(graph, remaining);
        ++waveNumber;
        std::size_t mainThreadSystems = 0;

        // Keep caller-affine work separate from every worker batch.
        for (const auto &name : wave)
        {
            const auto enabled = enabledSystems.find(name);
            if (enabled != enabledSystems.end() &&
                descriptors.at(name).threadAffinity() == SystemThreadAffinity::MainThread)
            {
                timings.at(name) =
                    invokeUpdate(enabled->second.get(), descriptors.at(name), registry, physics, frame, editMode);
                ++mainThreadSystems;
            }
        }

        std::vector<std::function<void()>> jobs;
        jobs.reserve(wave.size());
        std::string workerNames;
        std::atomic<std::size_t> active{0};
        std::atomic<std::size_t> peak{0};
        for (const auto &name : wave)
        {
            const auto enabled = enabledSystems.find(name);
            if (enabled == enabledSystems.end() ||
                descriptors.at(name).threadAffinity() == SystemThreadAffinity::MainThread)
            {
                continue;
            }

            ISystem *system = &enabled->second.get();
            const SystemDescriptor *descriptor = &descriptors.at(name);
            double *timing = &timings.at(name);
            if (logConcurrency)
            {
                if (!workerNames.empty())
                {
                    workerNames += ", ";
                }
                workerNames += name;
            }
            jobs.emplace_back(
                [&, system, descriptor, timing]
                {
                    if (!logConcurrency)
                    {
                        *timing = invokeUpdate(*system, *descriptor, registry, physics, frame, editMode);
                        return;
                    }

                    const auto count = active.fetch_add(1, std::memory_order_relaxed) + 1;
                    auto previous = peak.load(std::memory_order_relaxed);
                    while (previous < count &&
                        !peak.compare_exchange_weak(previous, count, std::memory_order_relaxed))
                    {
                    }
                    try
                    {
                        *timing = invokeUpdate(*system, *descriptor, registry, physics, frame, editMode);
                    }
                    catch (...)
                    {
                        active.fetch_sub(1, std::memory_order_relaxed);
                        throw;
                    }
                    active.fetch_sub(1, std::memory_order_relaxed);
                });
        }

        const auto batchSize = jobs.size();
        const auto logWave = [&](bool failed)
        {
            if (logConcurrency)
            {
                // A wide ready batch does not guarantee simultaneous execution.
                // Main-thread updates ran separately, at concurrency one.
                const auto observedPeak = std::max(mainThreadSystems > 0 ? std::size_t{1} : std::size_t{0},
                    peak.load(std::memory_order_relaxed));
                LOG_DEBUG(LogChannel::SystemScheduler,
                    "Wave {}: {} main-thread systems; worker batch {} [{}], {} workers; observed peak concurrency {}{}",
                    waveNumber,
                    mainThreadSystems,
                    batchSize,
                    workerNames,
                    m_jobs.workerCount(),
                    observedPeak,
                    failed ? " (failed; accepted work drained)" : "");
            }
        };

        // Batch failure drains accepted work before this call unwinds. No
        // successors are released unless all enabled updates in the wave finish.
        try
        {
            m_jobs.runBatchAndWait(std::move(jobs));
        }
        catch (...)
        {
            logWave(true);
            throw;
        }
        logWave(false);
        completeWave(graph, wave, remaining);
    }
    return timings;
}

SystemScheduler::DescriptorMap SystemScheduler::snapshotDescriptors(
    const SystemGraph &graph, const SystemMap &enabledSystems)
{
    DescriptorMap descriptors;
    std::vector<const ISystem *> instances;
    instances.reserve(enabledSystems.size());
    for (const auto &[name, system] : enabledSystems)
    {
        if (graph.successors().find(name) == graph.successors().end())
        {
            throw std::invalid_argument("Enabled system '" + name + "' is absent from the system graph");
        }
        if (std::find(instances.begin(), instances.end(), &system.get()) != instances.end())
        {
            throw std::invalid_argument("One system instance cannot occupy multiple system graph nodes");
        }

        auto descriptor = system.get().descriptor();
        if (descriptor.name() != name)
        {
            throw std::invalid_argument(
                "System graph name '" + name + "' does not match descriptor '" + descriptor.name() + "'");
        }
        descriptors.emplace(name, std::move(descriptor));
        instances.push_back(&system.get());
    }
    return descriptors;
}

SystemScheduler::PredecessorCounts SystemScheduler::countPredecessors(const SystemGraph &graph)
{
    PredecessorCounts remaining;
    for (const auto &name : graph.orderedSystems())
    {
        remaining.emplace(name, 0);
    }
    for (const auto &[name, successors] : graph.successors())
    {
        for (const auto &successor : successors)
        {
            ++remaining.at(successor);
        }
    }
    return remaining;
}

std::vector<std::string> SystemScheduler::readyWave(const SystemGraph &graph, const PredecessorCounts &remaining)
{
    std::vector<std::string> wave;
    for (const auto &name : graph.orderedSystems())
    {
        const auto pending = remaining.find(name);
        if (pending != remaining.end() && pending->second == 0)
        {
            wave.push_back(name);
        }
    }
    if (wave.empty())
    {
        throw std::logic_error("System graph has no ready nodes");
    }
    return wave;
}

void SystemScheduler::completeWave(
    const SystemGraph &graph, const std::vector<std::string> &wave, PredecessorCounts &remaining)
{
    for (const auto &name : wave)
    {
        remaining.erase(name);
        for (const auto &successor : graph.successors().at(name))
        {
            --remaining.at(successor);
        }
    }
}

double SystemScheduler::invokeUpdate(ISystem &system,
    const SystemDescriptor &descriptor,
    entt::registry &registry,
    Physics2D &physics,
    const FrameContext &frame,
    bool editMode)
{
    const auto start = std::chrono::steady_clock::now();
    {
        SystemContext context{registry, physics, frame, editMode};
        SystemEntities::UpdateScope access(context.entities, descriptor);
        system.update(context);
    }
    return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
}

double SystemScheduler::dispatchUpdate(ISystem &system,
    const SystemDescriptor &descriptor,
    entt::registry &registry,
    Physics2D &physics,
    const FrameContext &frame,
    bool editMode)
{
    double milliseconds = 0.0;
    const auto update = [&]
    {
        milliseconds = invokeUpdate(system, descriptor, registry, physics, frame, editMode);
    };
    if (descriptor.threadAffinity() == SystemThreadAffinity::MainThread)
    {
        update();
    }
    else
    {
        m_jobs.runAndWait(update);
    }
    return milliseconds;
}

void SystemScheduler::run(
    ISystem &system, entt::registry &registry, Physics2D &physics, const FrameContext &frame, bool editMode)
{
    const auto descriptor = system.descriptor();
    dispatchUpdate(system, descriptor, registry, physics, frame, editMode);
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
