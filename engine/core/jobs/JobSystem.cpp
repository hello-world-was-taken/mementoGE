#include "core/jobs/JobSystem.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <deque>
#include <exception>
#include <limits>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>

namespace
{
void terminateJobSystem(std::string_view reason, std::string_view detail = {}) noexcept
{
    // TODO: Add synchronous fatal logging to Sawyer and replace this stderr
    // fallback. It must flush before termination without joining the logger.
    // Sawyer writes asynchronously. Emit and flush a synchronous diagnostic
    // so termination cannot discard it or wait on logger callbacks.
    const auto printableLength = [](std::string_view text) noexcept
    {
        return static_cast<int>(std::min(text.size(), static_cast<std::size_t>(std::numeric_limits<int>::max())));
    };
    std::fprintf(stderr,
        "[FATAL] JobSystem: %.*s%s%.*s\n",
        printableLength(reason),
        reason.data(),
        detail.empty() ? "" : ": ",
        printableLength(detail),
        detail.empty() ? "" : detail.data());
    std::fflush(stderr);
    std::terminate();
}

// Identity is independent of the JobSystem's address. Groups can observe its
// lifetime without owning the pool or requiring shared ownership of JobSystem.
struct JobSystemIdentity
{
};
} // namespace

struct TaskGroup::State
{
    State(std::weak_ptr<const JobSystemIdentity> ownerIdentity, std::string groupName)
        : owner(std::move(ownerIdentity)), label(std::move(groupName))
    {
    }

    void addTask() noexcept
    {
        remaining.fetch_add(1, std::memory_order_relaxed);
    }

    void finishTask(std::exception_ptr error) noexcept
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            if (error != nullptr && firstException == nullptr)
            {
                firstException = std::move(error);
            }

            const std::size_t previous = remaining.fetch_sub(1, std::memory_order_release);
            if (previous == 0)
            {
                terminateJobSystem("Task group completed with no outstanding jobs", label);
            }
        }

        completion.notify_all();
    }

    void rethrowIfFailed() const
    {
        std::exception_ptr error;
        {
            std::lock_guard<std::mutex> lock(mutex);
            error = firstException;
        }

        if (error != nullptr)
        {
            std::rethrow_exception(error);
        }
    }

    const std::weak_ptr<const JobSystemIdentity> owner;
    const std::string label;
    std::atomic<std::size_t> remaining{0};
    mutable std::mutex mutex;
    std::condition_variable completion;
    std::exception_ptr firstException;
};

struct JobSystem::Impl
{
    struct ExecutionFrame
    {
        std::shared_ptr<const JobSystemIdentity> identity;
        WorkerIndex worker;
        const GroupList &groups;
        std::optional<std::reference_wrapper<const ExecutionFrame>> previous;
    };

    static thread_local std::optional<std::reference_wrapper<const ExecutionFrame>> currentExecution;

    class ScopedExecutionContext
    {
    public:
        ScopedExecutionContext(
            std::shared_ptr<const JobSystemIdentity> identity, WorkerIndex worker, const GroupList &groups) noexcept
            : m_frame{std::move(identity), worker, groups, currentExecution}
        {
            currentExecution = std::cref(m_frame);
        }

        ~ScopedExecutionContext()
        {
            currentExecution = m_frame.previous;
        }

        ScopedExecutionContext(const ScopedExecutionContext &) = delete;
        ScopedExecutionContext &operator=(const ScopedExecutionContext &) = delete;
        ScopedExecutionContext(ScopedExecutionContext &&) = delete;
        ScopedExecutionContext &operator=(ScopedExecutionContext &&) = delete;

    private:
        const ExecutionFrame m_frame;
    };

    struct QueuedTask
    {
        ErasedTask function;
        GroupList groups;
    };

    explicit Impl(JobSystemConfig jobConfig) : config(jobConfig), coordinatorThread(std::this_thread::get_id())
    {
        if (config.queueCapacity == 0)
        {
            throw std::invalid_argument("JobSystem queue capacity must be greater than zero");
        }

        workers.reserve(config.workerCount);
        try
        {
            for (std::size_t index = 0; index < config.workerCount; ++index)
            {
                workers.emplace_back(
                    [this, index]
                    {
                        workerLoop(WorkerIndex{index});
                    });
            }
        }
        catch (...)
        {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                acceptingExternal.store(false, std::memory_order_release);
            }
            workAvailable.notify_all();
            for (std::thread &worker : workers)
            {
                if (worker.joinable())
                {
                    worker.join();
                }
            }
            throw;
        }
    }

    bool isInternalSubmission() const noexcept
    {
        return currentExecution && currentExecution->get().identity == identity;
    }

    bool hasActiveTask() const noexcept
    {
        for (auto frame = currentExecution; frame; frame = frame->get().previous)
        {
            if (frame->get().identity == identity)
            {
                return true;
            }
        }
        return false;
    }

    bool groupContainsActiveTask(const std::shared_ptr<TaskGroup::State> &group) const noexcept
    {
        for (auto frame = currentExecution; frame; frame = frame->get().previous)
        {
            const ExecutionFrame &execution = frame->get();
            if (execution.identity == identity &&
                std::find(execution.groups.begin(), execution.groups.end(), group) != execution.groups.end())
            {
                return true;
            }
        }
        return false;
    }

    std::optional<WorkerIndex> currentWorkerIndex() const noexcept
    {
        return isInternalSubmission() ? std::optional<WorkerIndex>{currentExecution->get().worker} : std::nullopt;
    }

    bool mayAcceptSubmission(bool internal) const noexcept
    {
        return internal || acceptingExternal.load(std::memory_order_acquire);
    }

    void validateAndDeduplicateGroups(GroupList &groups) const
    {
        for (const auto &group : groups)
        {
            if (group == nullptr)
            {
                throw std::invalid_argument("Cannot submit a job to an invalid task group");
            }
            if (group->owner.lock() != identity)
            {
                throw std::invalid_argument("Cannot submit a job to another JobSystem's task group");
            }
        }

        if (groups.count == 2 && groups.states[0] == groups.states[1])
        {
            groups.states[1].reset();
            groups.count = 1;
        }
    }

    void enqueue(GroupList groups, ErasedTask function)
    {
        validateAndDeduplicateGroups(groups);
        QueuedTask task{std::move(function), std::move(groups)};
        const bool internal = isInternalSubmission();

        if (workers.empty())
        {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!mayAcceptSubmission(internal))
                {
                    throw std::runtime_error("JobSystem is closed");
                }

                for (const auto &group : task.groups)
                {
                    group->addTask();
                }
                inFlight.fetch_add(1, std::memory_order_release);
            }

            execute(std::move(task), WorkerIndex{0});
            return;
        }

        std::unique_lock<std::mutex> lock(queueMutex);
        while (queue.size() >= config.queueCapacity)
        {
            if (!mayAcceptSubmission(internal))
            {
                throw std::runtime_error("JobSystem is closed");
            }

            if (internal)
            {
                const WorkerIndex worker = currentExecution->get().worker;
                lock.unlock();
                if (!tryExecuteOne(worker))
                {
                    std::this_thread::yield();
                }
                lock.lock();
            }
            else
            {
                queueHasRoom.wait(lock,
                    [this]
                    {
                        return queue.size() < config.queueCapacity ||
                               !acceptingExternal.load(std::memory_order_acquire);
                    });
            }
        }

        if (!mayAcceptSubmission(internal))
        {
            throw std::runtime_error("JobSystem is closed");
        }

        // Queue insertion is the only throwing operation after acceptance is
        // checked. Counters are published only after it succeeds and before a
        // worker can observe the new task.
        queue.push_back(std::move(task));
        for (const auto &group : queue.back().groups)
        {
            group->addTask();
        }
        inFlight.fetch_add(1, std::memory_order_release);
        peakQueued = std::max(peakQueued, queue.size());

        lock.unlock();
        workAvailable.notify_one();
    }

    bool tryExecuteOne(WorkerIndex worker)
    {
        QueuedTask task;
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            if (queue.empty())
            {
                return false;
            }

            task = std::move(queue.front());
            queue.pop_front();
        }

        queueHasRoom.notify_one();
        execute(std::move(task), worker);
        return true;
    }

    void execute(QueuedTask task, WorkerIndex worker) noexcept
    {
        std::exception_ptr error;
        {
            ScopedExecutionContext context(identity, worker, task.groups);
            try
            {
                task.function(worker);
            }
            catch (...)
            {
                error = std::current_exception();
            }
        }

        // The completion predicates observed through workAvailable/drained
        // are changed while holding queueMutex. This avoids a lost wake-up
        // between another thread checking its predicate and going to sleep.
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            for (const auto &group : task.groups)
            {
                group->finishTask(error);
            }

            const std::size_t previous = inFlight.fetch_sub(1, std::memory_order_acq_rel);
            if (previous == 0)
            {
                terminateJobSystem("In-flight job counter underflow");
            }
        }

        workAvailable.notify_all();
        drained.notify_all();
    }

    void workerLoop(WorkerIndex worker)
    {
        while (true)
        {
            QueuedTask task;
            {
                std::unique_lock<std::mutex> lock(queueMutex);
                workAvailable.wait(lock,
                    [this]
                    {
                        return !queue.empty() || (!acceptingExternal.load(std::memory_order_acquire) &&
                                                     inFlight.load(std::memory_order_acquire) == 0);
                    });

                if (queue.empty())
                {
                    return;
                }

                task = std::move(queue.front());
                queue.pop_front();
            }

            queueHasRoom.notify_one();
            execute(std::move(task), worker);
        }
    }

    void wait(const std::shared_ptr<TaskGroup::State> &group)
    {
        if (group == nullptr)
        {
            throw std::invalid_argument("Cannot wait on an invalid task group");
        }
        if (group->owner.lock() != identity)
        {
            throw std::invalid_argument("Cannot wait on another JobSystem's task group");
        }

        // Helping may nest jobs on this thread. Waiting for a group containing
        // any suspended ancestor also deadlocks, so inspect the whole chain.
        if (groupContainsActiveTask(group))
        {
            throw std::logic_error("A job cannot wait on a task group containing itself or a suspended ancestor");
        }

        const bool executingPoolTask = isInternalSubmission();
        const bool coordinatorMayHelp =
            config.callerHelpsWhileWaiting && std::this_thread::get_id() == coordinatorThread;
        const bool shouldHelp = executingPoolTask || coordinatorMayHelp;

        if (shouldHelp)
        {
            const WorkerIndex helper = executingPoolTask ? currentExecution->get().worker : WorkerIndex{workers.size()};

            while (group->remaining.load(std::memory_order_acquire) != 0)
            {
                if (tryExecuteOne(helper))
                {
                    continue;
                }

                std::unique_lock<std::mutex> lock(queueMutex);
                workAvailable.wait(lock,
                    [this, &group]
                    {
                        return !queue.empty() || group->remaining.load(std::memory_order_acquire) == 0;
                    });
            }
        }
        else
        {
            std::unique_lock<std::mutex> lock(group->mutex);
            group->completion.wait(lock,
                [&group]
                {
                    return group->remaining.load(std::memory_order_acquire) == 0;
                });
        }

        group->rethrowIfFailed();
    }

    void close() noexcept
    {
        // Serialize the acceptance boundary with enqueue(). Once close()
        // returns, no external submitter can still pass the open check.
        {
            std::lock_guard<std::mutex> lock(queueMutex);
            acceptingExternal.store(false, std::memory_order_release);
        }
        workAvailable.notify_all();
        queueHasRoom.notify_all();
    }

    void join()
    {
        if (hasActiveTask())
        {
            throw std::logic_error("JobSystem::join cannot be called from one of its jobs");
        }

        close();

        std::lock_guard<std::mutex> joinLock(joinMutex);
        if (joined)
        {
            return;
        }

        for (std::thread &worker : workers)
        {
            if (worker.joinable())
            {
                worker.join();
            }
        }

        std::unique_lock<std::mutex> queueLock(queueMutex);
        drained.wait(queueLock,
            [this]
            {
                return inFlight.load(std::memory_order_acquire) == 0;
            });
        joined = true;
    }

    const std::shared_ptr<const JobSystemIdentity> identity = std::make_shared<const JobSystemIdentity>();
    const JobSystemConfig config;
    const std::thread::id coordinatorThread;

    std::vector<std::thread> workers;
    std::deque<QueuedTask> queue;

    mutable std::mutex queueMutex;
    std::condition_variable workAvailable;
    std::condition_variable queueHasRoom;
    std::condition_variable drained;

    std::atomic<bool> acceptingExternal{true};
    std::atomic<std::size_t> inFlight{0};
    std::size_t peakQueued = 0;

    std::mutex joinMutex;
    bool joined = false;
};

thread_local std::optional<std::reference_wrapper<const JobSystem::Impl::ExecutionFrame>>
    JobSystem::Impl::currentExecution;

std::size_t JobSystemConfig::recommendedWorkerCount() noexcept
{
    const unsigned int hardwareThreads = std::thread::hardware_concurrency();
    return hardwareThreads > 1 ? hardwareThreads - 1 : 0;
}

TaskGroup::TaskGroup(std::shared_ptr<State> state) : m_state(std::move(state))
{
}

bool TaskGroup::valid() const noexcept
{
    return m_state != nullptr;
}

bool TaskGroup::isComplete() const noexcept
{
    return m_state == nullptr || m_state->remaining.load(std::memory_order_acquire) == 0;
}

std::string TaskGroup::name() const
{
    return m_state != nullptr ? m_state->label : std::string{};
}

JobSystem::JobSystem(JobSystemConfig config) : m_impl(std::make_unique<Impl>(config))
{
}

JobSystem::~JobSystem()
{
    if (m_impl == nullptr)
    {
        return;
    }

    m_impl->close();
    try
    {
        m_impl->join();
    }
    catch (const std::exception &error)
    {
        terminateJobSystem("Shutdown failed", error.what());
    }
    catch (...)
    {
        terminateJobSystem("Shutdown failed with an unknown exception");
    }
}

TaskGroup JobSystem::createGroup(std::string name)
{
    return TaskGroup(std::make_shared<TaskGroup::State>(m_impl->identity, std::move(name)));
}

void JobSystem::enqueue(GroupList groups, ErasedTask task)
{
    m_impl->enqueue(std::move(groups), std::move(task));
}

void JobSystem::wait(const TaskGroup &group)
{
    m_impl->wait(group.m_state);
}

void JobSystem::close() noexcept
{
    m_impl->close();
}

void JobSystem::join()
{
    m_impl->join();
}

bool JobSystem::isOpen() const noexcept
{
    return m_impl->acceptingExternal.load(std::memory_order_acquire);
}

std::size_t JobSystem::workerCount() const noexcept
{
    return m_impl->workers.size();
}

std::size_t JobSystem::executionSlotCount() const noexcept
{
    if (m_impl->workers.empty())
    {
        return 1;
    }

    return m_impl->workers.size() + (m_impl->config.callerHelpsWhileWaiting ? 1 : 0);
}

std::size_t JobSystem::queuedTaskCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    return m_impl->queue.size();
}

std::size_t JobSystem::peakQueuedTaskCount() const noexcept
{
    std::lock_guard<std::mutex> lock(m_impl->queueMutex);
    return m_impl->peakQueued;
}

std::optional<WorkerIndex> JobSystem::currentWorkerIndex() const noexcept
{
    return m_impl->currentWorkerIndex();
}
