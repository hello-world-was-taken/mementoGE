#pragma once

#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

class JobSystem;

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

// A copyable handle to a collection of jobs. The underlying state stays alive
// until every submitted job has completed, even if all public handles are
// destroyed first.
class TaskGroup
{
public:
    TaskGroup() = default;

    bool valid() const noexcept;
    bool isComplete() const noexcept;
    std::string name() const;

private:
    struct State;

    explicit TaskGroup(std::shared_ptr<State> state);

    std::shared_ptr<State> m_state;

    friend class JobSystem;
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

    TaskGroup createGroup(std::string name = {});

    template <typename Function> void submit(const TaskGroup &group, Function &&function)
    {
        enqueue(GroupList{{group.m_state}, 1}, eraseTask(std::forward<Function>(function)));
    }

    // Associate a job with both a work group and a wider lifetime group.
    // Waiting on either group observes the same job and any captured error.
    template <typename Function>
    void submit(const TaskGroup &group, const TaskGroup &lifetimeGroup, Function &&function)
    {
        enqueue(GroupList{{group.m_state, lifetimeGroup.m_state}, 2}, eraseTask(std::forward<Function>(function)));
    }

    template <typename Function>
    void parallelFor(const TaskGroup &group, std::size_t count, std::size_t minimumGrainSize, Function &&function)
    {
        parallelForImpl(group, std::nullopt, count, minimumGrainSize, std::forward<Function>(function));
    }

    template <typename Function>
    void parallelFor(const TaskGroup &group,
        const TaskGroup &lifetimeGroup,
        std::size_t count,
        std::size_t minimumGrainSize,
        Function &&function)
    {
        parallelForImpl(group, std::cref(lifetimeGroup), count, minimumGrainSize, std::forward<Function>(function));
    }

    // Wait rethrows the first exception captured by the group. A pool worker
    // waiting for child jobs helps execute queued work to avoid starvation.
    void wait(const TaskGroup &group);

    // close() rejects new root submissions while allowing already-running
    // jobs to submit required child work. join() drains accepted work and
    // joins every worker. Both calls are idempotent.
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

    // A submitted job belongs to one or two groups; keep their handles inline.
    struct GroupList
    {
        std::array<std::shared_ptr<TaskGroup::State>, 2> states{};
        std::size_t count = 0;

        auto begin() const noexcept
        {
            return states.begin();
        }

        auto end() const noexcept
        {
            return states.begin() + count;
        }
    };

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    void enqueue(GroupList groups, ErasedTask task);

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

    template <typename Function>
    void parallelForImpl(const TaskGroup &group,
        std::optional<std::reference_wrapper<const TaskGroup>> lifetimeGroup,
        std::size_t count,
        std::size_t minimumGrainSize,
        Function &&function)
    {
        if (minimumGrainSize == 0)
        {
            throw std::invalid_argument("JobSystem::parallelFor requires a non-zero grain size");
        }

        if (count == 0)
        {
            return;
        }

        using Callable = std::decay_t<Function>;
        static_assert(std::is_invocable_v<const Callable &, std::size_t, std::size_t, WorkerIndex> ||
                          std::is_invocable_v<const Callable &, std::size_t, std::size_t>,
            "A parallel-for body must accept (begin, end, WorkerIndex) or "
            "(begin, end) and be const-invocable");

        auto body = std::make_shared<Callable>(std::forward<Function>(function));

        for (std::size_t begin = 0; begin < count;)
        {
            const std::size_t remaining = count - begin;
            const std::size_t end = remaining <= minimumGrainSize ? count : begin + minimumGrainSize;

            auto rangeJob = [body, begin, end](WorkerIndex worker)
            {
                const Callable &callable = *body;
                if constexpr (std::is_invocable_v<const Callable &, std::size_t, std::size_t, WorkerIndex>)
                {
                    std::invoke(callable, begin, end, worker);
                }
                else
                {
                    std::invoke(callable, begin, end);
                }
            };

            if (lifetimeGroup)
            {
                submit(group, lifetimeGroup->get(), std::move(rangeJob));
            }
            else
            {
                submit(group, std::move(rangeJob));
            }

            begin = end;
        }
    }
};
