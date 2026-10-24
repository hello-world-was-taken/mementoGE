#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>
#include <vector>

struct WorkerIndex
{
    std::size_t value = 0;

    friend constexpr bool operator==(WorkerIndex lhs, WorkerIndex rhs) noexcept
    {
        return lhs.value == rhs.value;
    }

    friend constexpr bool operator!=(WorkerIndex lhs, WorkerIndex rhs) noexcept
    {
        return !(lhs == rhs);
    }
};

struct JobSystemConfig
{
    static std::size_t recommendedWorkerCount() noexcept;

    // Zero workers executes submitted work synchronously on the caller. This
    // is useful for deterministic tests and platforms where threads are not
    // desirable.
    std::size_t workerCount = recommendedWorkerCount();
    std::size_t queueCapacity = 4096;
    bool callerHelpsWhileWaiting = true;
};

class JobSystem
{
public:
    explicit JobSystem(JobSystemConfig config = {});
    ~JobSystem();

    JobSystem(const JobSystem &) = delete;
    JobSystem &operator=(const JobSystem &) = delete;
    JobSystem(JobSystem &&) = delete;
    JobSystem &operator=(JobSystem &&) = delete;

    // Runs one job and waits for it to finish, rethrowing its exception on
    // the caller. A waiting pool worker helps execute queued jobs so nested
    // calls also work with a single worker.
    template <typename Function> void runAndWait(Function &&function)
    {
        runAndWaitErased(eraseTask(std::forward<Function>(function)));
    }

    // Runs a finite batch of independent jobs and waits for every accepted
    // job, even on failure. Jobs may overlap when workers are available; zero
    // workers runs them inline. An empty batch returns immediately.
    // The first job exception in input order is rethrown after the batch finishes.
    // If submission fails, drain accepted jobs before reporting that failure.
    template <typename Function> void runBatchAndWait(std::vector<Function> jobs)
    {
        std::vector<ErasedTask> tasks;
        tasks.reserve(jobs.size());
        for (auto &job : jobs)
        {
            tasks.push_back(eraseTask(std::move(job)));
        }

        // Destroy moved-from callables before dispatch too: a custom move
        // constructor can leave captures behind in the original object.
        jobs.clear();
        runBatchAndWaitErased(std::move(tasks));
    }

    // close() rejects new root calls while allowing already-running jobs to
    // finish nested runAndWait()/runBatchAndWait() calls. join() drains
    // accepted work and joins every worker. Both calls are idempotent.
    void close() noexcept;
    void join();

    bool isOpen() const noexcept;
    std::size_t workerCount() const noexcept;
    std::size_t executionSlotCount() const noexcept;
    std::size_t queuedTaskCount() const noexcept;
    std::size_t peakQueuedTaskCount() const noexcept;
    std::optional<WorkerIndex> currentWorkerIndex() const noexcept;

private:
    using ErasedTask = std::function<void(WorkerIndex)>;

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    void runAndWaitErased(ErasedTask task);
    void runBatchAndWaitErased(std::vector<ErasedTask> tasks);

    template <typename Function> static ErasedTask eraseTask(Function &&function)
    {
        using Callable = std::decay_t<Function>;
        static_assert(std::is_invocable_v<Callable &, WorkerIndex> || std::is_invocable_v<Callable &>,
            "A job must be callable with WorkerIndex or with no arguments");

        // std::function requires a copyable target. Keeping the user's
        // callable in shared storage also supports move-only captures.
        auto callable = std::make_shared<Callable>(std::forward<Function>(function));

        return [callable = std::move(callable)](WorkerIndex worker)
        {
            if constexpr (std::is_invocable_v<Callable &, WorkerIndex>)
            {
                std::invoke(*callable, worker);
            }
            else
            {
                std::invoke(*callable);
            }
        };
    }
};
