#include "core/jobs/JobSystem.h"

#include <algorithm>
#include <atomic>
#include <condition_variable>
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

} // namespace

struct JobSystem::Impl
{
    struct ExecutionFrame
    {
        const Impl *owner;
        WorkerIndex worker;
        std::optional<std::reference_wrapper<const ExecutionFrame>> previous;
    };

    static thread_local std::optional<std::reference_wrapper<const ExecutionFrame>> currentExecution;

    class ScopedExecutionContext
    {
    public:
        ScopedExecutionContext(const Impl &owner, WorkerIndex worker) noexcept
            : m_frame{&owner, worker, currentExecution}
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

    // Each runAndWait() owns this state until its queued job has finished.
    // queueMutex protects both fields and workAvailable signals completion.
    struct Completion
    {
        bool finished = false;
        std::exception_ptr error;
    };

    struct QueuedTask
    {
        ErasedTask function;
        Completion *completion = nullptr;
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

    std::optional<WorkerIndex> activeWorkerIndex() const noexcept
    {
        // An inline call into another pool may suspend this pool's job.
        // Its worker must still help when that nested call returns work here.
        for (auto frame = currentExecution; frame; frame = frame->get().previous)
        {
            if (frame->get().owner == this)
            {
                return frame->get().worker;
            }
        }
        return std::nullopt;
    }

    std::optional<WorkerIndex> currentWorkerIndex() const noexcept
    {
        if (currentExecution && currentExecution->get().owner == this)
        {
            return currentExecution->get().worker;
        }
        return std::nullopt;
    }

    bool mayAcceptSubmission(bool internal) const noexcept
    {
        return internal || acceptingExternal.load(std::memory_order_acquire);
    }

    void runAndWait(ErasedTask function)
    {
        Completion completion;
        enqueue(QueuedTask{std::move(function), &completion});
        wait(completion);
    }

    void enqueue(QueuedTask task)
    {
        const auto activeWorker = activeWorkerIndex();
        const bool internal = activeWorker.has_value();

        if (workers.empty())
        {
            {
                std::lock_guard<std::mutex> lock(queueMutex);
                if (!mayAcceptSubmission(internal))
                {
                    throw std::runtime_error("JobSystem is closed");
                }

                ++inFlight;
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
                lock.unlock();
                tryExecuteOne(*activeWorker);
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

        // Publish acceptance only after queue insertion succeeds, before a
        // worker can observe the new job.
        queue.push_back(std::move(task));
        ++inFlight;
        peakQueued = std::max(peakQueued, queue.size());

        lock.unlock();
        workAvailable.notify_all();
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
            ScopedExecutionContext context(*this, worker);
            try
            {
                task.function(worker);
            }
            catch (...)
            {
                error = std::current_exception();
            }

            // Release captures before publishing completion to the caller.
            task.function = {};
        }

        {
            std::lock_guard<std::mutex> lock(queueMutex);
            task.completion->error = std::move(error);
            task.completion->finished = true;
            --inFlight;
        }

        workAvailable.notify_all();
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
                        return !queue.empty() || (!acceptingExternal.load(std::memory_order_acquire) && inFlight == 0);
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

    void wait(Completion &completion)
    {
        const auto activeWorker = activeWorkerIndex();
        const bool executingPoolTask = activeWorker.has_value();
        const bool coordinatorMayHelp =
            config.callerHelpsWhileWaiting && std::this_thread::get_id() == coordinatorThread;
        const bool shouldHelp = executingPoolTask || coordinatorMayHelp;
        const WorkerIndex helper = activeWorker.value_or(WorkerIndex{workers.size()});

        std::unique_lock<std::mutex> lock(queueMutex);
        while (!completion.finished)
        {
            if (shouldHelp && !queue.empty())
            {
                lock.unlock();
                tryExecuteOne(helper);
                lock.lock();
            }
            else
            {
                workAvailable.wait(lock,
                    [this, &completion, shouldHelp]
                    {
                        return completion.finished || (shouldHelp && !queue.empty());
                    });
            }
        }

        const std::exception_ptr error = completion.error;
        lock.unlock();
        if (error != nullptr)
        {
            std::rethrow_exception(error);
        }
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
        if (activeWorkerIndex().has_value())
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
        workAvailable.wait(queueLock,
            [this]
            {
                return inFlight == 0;
            });
        joined = true;
    }

    const JobSystemConfig config;
    const std::thread::id coordinatorThread;

    std::vector<std::thread> workers;
    std::deque<QueuedTask> queue;

    mutable std::mutex queueMutex;
    std::condition_variable workAvailable;
    std::condition_variable queueHasRoom;

    std::atomic<bool> acceptingExternal{true};
    std::size_t inFlight = 0;
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

void JobSystem::runAndWaitErased(ErasedTask task)
{
    m_impl->runAndWait(std::move(task));
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
