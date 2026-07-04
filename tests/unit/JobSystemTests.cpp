#include "core/jobs/JobSystem.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

class JobSystemRunTests : public ::testing::TestWithParam<std::size_t>
{
};

TEST_P(JobSystemRunTests, RunsOneJobOnTheConfiguredExecutionThread)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 8, false});
    const std::thread::id caller = std::this_thread::get_id();
    std::thread::id executionThread;
    std::optional<WorkerIndex> executionSlot;
    int result = 0;

    jobs.runAndWait(
        [&](WorkerIndex worker)
        {
            executionThread = std::this_thread::get_id();
            executionSlot = jobs.currentWorkerIndex();
            EXPECT_EQ(executionSlot, std::optional<WorkerIndex>{worker});
            result = 42;
        });

    EXPECT_EQ(result, 42);
    ASSERT_TRUE(executionSlot.has_value());
    EXPECT_LT(executionSlot->value, jobs.executionSlotCount());
    EXPECT_EQ(jobs.workerCount(), GetParam());
    EXPECT_FALSE(jobs.currentWorkerIndex().has_value());
    if (GetParam() == 0)
    {
        EXPECT_EQ(executionThread, caller);
    }
    else
    {
        EXPECT_NE(executionThread, caller);
    }
}

TEST_P(JobSystemRunTests, DoesNotReturnUntilTheJobFinishes)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 8, false});
    std::promise<void> started;
    std::promise<void> release;
    const std::shared_future<void> released = release.get_future().share();
    auto call = std::async(std::launch::async,
        [&]
        {
            jobs.runAndWait(
                [&]
                {
                    started.set_value();
                    released.wait();
                });
        });

    started.get_future().wait();
    EXPECT_EQ(call.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
    release.set_value();
    EXPECT_NO_THROW(call.get());
}

TEST_P(JobSystemRunTests, RethrowsTheJobExceptionAndKeepsThePoolUsable)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 8, false});
    EXPECT_THROW(jobs.runAndWait(
                     []
                     {
                         throw std::runtime_error("job failed");
                     }),
        std::runtime_error);

    int result = 0;
    EXPECT_NO_THROW(jobs.runAndWait(
        [&]
        {
            result = 7;
        }));
    EXPECT_EQ(result, 7);
}

TEST_P(JobSystemRunTests, SupportsMoveOnlyCapturesAndReleasesThemBeforeReturning)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 8, false});
    bool released = false;
    const auto deleter = [&](int *value)
    {
        delete value;
        released = true;
    };
    std::unique_ptr<int, decltype(deleter)> owned(new int(42), deleter);
    int result = 0;

    jobs.runAndWait(
        [value = std::move(owned), &result]
        {
            result = *value;
        });

    EXPECT_EQ(result, 42);
    EXPECT_TRUE(released);
}

TEST_P(JobSystemRunTests, NestedCallsFinishWithoutWorkerStarvation)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 1, false});
    int result = 0;
    jobs.runAndWait(
        [&]
        {
            jobs.runAndWait(
                [&]
                {
                    jobs.runAndWait(
                        [&]
                        {
                            result = 42;
                        });
                });
        });

    EXPECT_EQ(result, 42);
    EXPECT_EQ(jobs.queuedTaskCount(), 0U);
}

TEST_P(JobSystemRunTests, ConcurrentCallsKeepCompletionAndExceptionsSeparate)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 2, false});
    std::atomic<int> finished{0};
    std::vector<std::future<bool>> calls;
    for (int index = 0; index < 16; ++index)
    {
        calls.push_back(std::async(std::launch::async,
            [&, index]
            {
                try
                {
                    jobs.runAndWait(
                        [&, index]
                        {
                            finished.fetch_add(1, std::memory_order_relaxed);
                            if (index % 2 == 0)
                            {
                                throw std::runtime_error("expected failure");
                            }
                        });
                    return index % 2 != 0;
                }
                catch (const std::runtime_error &)
                {
                    return index % 2 == 0;
                }
            }));
    }

    for (auto &call : calls)
    {
        EXPECT_TRUE(call.get());
    }
    EXPECT_EQ(finished.load(std::memory_order_relaxed), 16);
    EXPECT_LE(jobs.peakQueuedTaskCount(), 2U);
}

TEST_P(JobSystemRunTests, RejectsJoinFromItsOwnJob)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 8, false});
    jobs.runAndWait(
        [&]
        {
            EXPECT_THROW(jobs.join(), std::logic_error);
        });
    EXPECT_TRUE(jobs.isOpen());
    EXPECT_NO_THROW(jobs.join());
}

TEST_P(JobSystemRunTests, CloseRejectsNewCallsAndJoinIsIdempotent)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 8, false});
    jobs.close();
    jobs.close();
    EXPECT_FALSE(jobs.isOpen());
    EXPECT_THROW(jobs.runAndWait(
                     []
                     {
                     }),
        std::runtime_error);
    EXPECT_NO_THROW(jobs.join());
    EXPECT_NO_THROW(jobs.join());
}

TEST_P(JobSystemRunTests, RunningJobsCanFinishNestedCallsAfterClose)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 1, false});
    std::promise<void> started;
    std::promise<void> release;
    const std::shared_future<void> released = release.get_future().share();
    bool nestedFinished = false;
    auto call = std::async(std::launch::async,
        [&]
        {
            jobs.runAndWait(
                [&]
                {
                    started.set_value();
                    released.wait();
                    jobs.runAndWait(
                        [&]
                        {
                            nestedFinished = true;
                        });
                });
        });

    started.get_future().wait();
    jobs.close();
    EXPECT_THROW(jobs.runAndWait(
                     []
                     {
                     }),
        std::runtime_error);
    release.set_value();
    EXPECT_NO_THROW(call.get());
    EXPECT_NO_THROW(jobs.join());
    EXPECT_TRUE(nestedFinished);
}

TEST_P(JobSystemRunTests, JoinWaitsForAnAcceptedCallToFinish)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 8, false});
    std::promise<void> started;
    std::promise<void> release;
    const std::shared_future<void> released = release.get_future().share();
    auto call = std::async(std::launch::async,
        [&]
        {
            jobs.runAndWait(
                [&]
                {
                    started.set_value();
                    released.wait();
                });
        });
    started.get_future().wait();

    auto joined = std::async(std::launch::async,
        [&]
        {
            jobs.join();
        });
    EXPECT_EQ(joined.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
    release.set_value();
    EXPECT_NO_THROW(joined.get());
    EXPECT_NO_THROW(call.get());
    EXPECT_FALSE(jobs.isOpen());
}

INSTANTIATE_TEST_SUITE_P(ZeroOneAndTwoWorkers, JobSystemRunTests, ::testing::Values(0U, 1U, 2U));

TEST(JobSystemTests, ReusesItsWorkerAcrossCalls)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    std::thread::id firstWorker;
    jobs.runAndWait(
        [&]
        {
            firstWorker = std::this_thread::get_id();
        });
    for (int iteration = 0; iteration < 16; ++iteration)
    {
        jobs.runAndWait(
            [&]
            {
                EXPECT_EQ(std::this_thread::get_id(), firstWorker);
            });
    }
}

TEST(JobSystemTests, QueueBackpressureRejectsUnacceptedCallsOnCloseAndDrainsAcceptedWork)
{
    JobSystem jobs(JobSystemConfig{1, 1, false});
    std::promise<void> firstStarted;
    std::promise<void> releaseFirst;
    const std::shared_future<void> released = releaseFirst.get_future().share();
    auto first = std::async(std::launch::async,
        [&]
        {
            jobs.runAndWait(
                [&]
                {
                    firstStarted.set_value();
                    released.wait();
                });
        });
    firstStarted.get_future().wait();

    bool secondFinished = false;
    auto second = std::async(std::launch::async,
        [&]
        {
            jobs.runAndWait(
                [&]
                {
                    secondFinished = true;
                });
        });
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(2);
    while (jobs.queuedTaskCount() == 0 && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    EXPECT_EQ(jobs.queuedTaskCount(), 1U);

    std::promise<void> thirdStarted;
    auto third = std::async(std::launch::async,
        [&]
        {
            thirdStarted.set_value();
            try
            {
                jobs.runAndWait(
                    []
                    {
                    });
                return false;
            }
            catch (const std::runtime_error &)
            {
                return true;
            }
        });
    thirdStarted.get_future().wait();
    EXPECT_EQ(third.wait_for(std::chrono::milliseconds(20)), std::future_status::timeout);
    jobs.close();
    EXPECT_TRUE(third.get());
    releaseFirst.set_value();
    EXPECT_NO_THROW(first.get());
    EXPECT_NO_THROW(second.get());
    EXPECT_NO_THROW(jobs.join());
    EXPECT_TRUE(secondFinished);
    EXPECT_EQ(jobs.peakQueuedTaskCount(), 1U);
    EXPECT_EQ(jobs.queuedTaskCount(), 0U);
}

TEST(JobSystemTests, CoordinatorCanHelpWhileItsWorkerIsOccupied)
{
    JobSystem jobs(JobSystemConfig{1, 8, true});
    std::promise<void> workerStarted;
    std::promise<void> releaseWorker;
    const std::shared_future<void> released = releaseWorker.get_future().share();
    auto occupied = std::async(std::launch::async,
        [&]
        {
            jobs.runAndWait(
                [&]
                {
                    workerStarted.set_value();
                    released.wait();
                });
        });
    workerStarted.get_future().wait();

    const std::thread::id coordinator = std::this_thread::get_id();
    jobs.runAndWait(
        [&](WorkerIndex worker)
        {
            EXPECT_EQ(std::this_thread::get_id(), coordinator);
            EXPECT_EQ(worker.value, 1U);
        });
    releaseWorker.set_value();
    EXPECT_NO_THROW(occupied.get());
    EXPECT_EQ(jobs.executionSlotCount(), 2U);
}

TEST(JobSystemTests, NestedPoolsRestoreExecutionContextAfterAnException)
{
    JobSystem outerJobs(JobSystemConfig{0, 8, false});
    JobSystem innerJobs(JobSystemConfig{0, 8, false});
    outerJobs.runAndWait(
        [&](WorkerIndex outerWorker)
        {
            EXPECT_EQ(outerJobs.currentWorkerIndex(), std::optional<WorkerIndex>{outerWorker});
            EXPECT_THROW(innerJobs.runAndWait(
                             [&](WorkerIndex innerWorker)
                             {
                                 EXPECT_FALSE(outerJobs.currentWorkerIndex().has_value());
                                 EXPECT_EQ(innerJobs.currentWorkerIndex(), std::optional<WorkerIndex>{innerWorker});
                                 EXPECT_THROW(outerJobs.join(), std::logic_error);
                                 throw std::runtime_error("inner failure");
                             }),
                std::runtime_error);
            EXPECT_EQ(outerJobs.currentWorkerIndex(), std::optional<WorkerIndex>{outerWorker});
            EXPECT_FALSE(innerJobs.currentWorkerIndex().has_value());
        });

    EXPECT_FALSE(outerJobs.currentWorkerIndex().has_value());
    EXPECT_FALSE(innerJobs.currentWorkerIndex().has_value());
}

TEST(JobSystemTests, NestedPoolsCanReturnWorkToASuspendedWorkerAfterClose)
{
    JobSystem outerJobs(JobSystemConfig{1, 1, false});
    JobSystem innerJobs(JobSystemConfig{0, 1, false});
    int result = 0;

    outerJobs.runAndWait(
        [&](WorkerIndex outerWorker)
        {
            innerJobs.runAndWait(
                [&]
                {
                    EXPECT_FALSE(outerJobs.currentWorkerIndex().has_value());
                    outerJobs.close();
                    outerJobs.runAndWait(
                        [&](WorkerIndex nestedWorker)
                        {
                            EXPECT_EQ(nestedWorker, outerWorker);
                            result = 42;
                        });
                    EXPECT_FALSE(outerJobs.currentWorkerIndex().has_value());
                });
            EXPECT_EQ(outerJobs.currentWorkerIndex(), std::optional<WorkerIndex>{outerWorker});
        });

    EXPECT_EQ(result, 42);
    EXPECT_FALSE(outerJobs.isOpen());
    EXPECT_NO_THROW(outerJobs.join());
}

TEST(JobSystemTests, InvalidQueueCapacityIsRejected)
{
    EXPECT_THROW(JobSystem(JobSystemConfig{0, 0, false}), std::invalid_argument);
}

TEST(JobSystemTests, FatalShutdownFailurePrintsADiagnosticBeforeTerminating)
{
    EXPECT_DEATH(
        {
            auto jobs = std::make_unique<JobSystem>(JobSystemConfig{0, 8, false});
            jobs->runAndWait(
                [&]
                {
                    jobs.reset();
                });
        },
        "\\[FATAL\\] JobSystem: Shutdown failed: JobSystem::join cannot be called from one of its jobs");
}
