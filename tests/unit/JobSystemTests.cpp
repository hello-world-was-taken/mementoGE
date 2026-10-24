#include "core/jobs/JobSystem.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>
#include <thread>
#include <utility>
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

TEST_P(JobSystemRunTests, EmptyBatchesSucceedBeforeAndAfterShutdown)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 2, false});
    const std::vector<std::function<void()>> empty;

    EXPECT_NO_THROW(jobs.runBatchAndWait(empty));
    jobs.close();
    EXPECT_NO_THROW(jobs.runBatchAndWait(empty));
    jobs.join();
    EXPECT_NO_THROW(jobs.runBatchAndWait(empty));
    EXPECT_EQ(jobs.queuedTaskCount(), 0u);
}

TEST_P(JobSystemRunTests, BatchesCompleteEveryJobOnValidExecutionSlotsWithoutExceedingQueueCapacity)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 2, false});
    const auto caller = std::this_thread::get_id();
    std::vector<int> results(12, 0);
    std::vector<std::thread::id> threads(results.size());
    std::vector<std::function<void(WorkerIndex)>> batch;
    for (std::size_t index = 0; index < results.size(); ++index)
    {
        batch.emplace_back([&, index](WorkerIndex worker)
            {
                EXPECT_EQ(jobs.currentWorkerIndex(), std::optional<WorkerIndex>{worker});
                EXPECT_LT(worker.value, jobs.executionSlotCount());
                results[index] = static_cast<int>(index + 1);
                threads[index] = std::this_thread::get_id();
            });
    }

    jobs.runBatchAndWait(std::move(batch));

    for (std::size_t index = 0; index < results.size(); ++index)
    {
        EXPECT_EQ(results[index], static_cast<int>(index + 1));
        if (GetParam() == 0)
        {
            EXPECT_EQ(threads[index], caller);
        }
        else
        {
            EXPECT_NE(threads[index], caller);
        }
    }
    EXPECT_LE(jobs.peakQueuedTaskCount(), 2u);
    EXPECT_EQ(jobs.queuedTaskCount(), 0u);
}

TEST_P(JobSystemRunTests, BatchJobFailuresDoNotSkipLaterJobsAndReportTheFirstInputFailure)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 2, false});
    std::atomic<int> finished{0};
    std::vector<std::function<void()>> batch;
    for (int index = 0; index < 9; ++index)
    {
        batch.emplace_back([&, index]
            {
                finished.fetch_add(1, std::memory_order_relaxed);
                if (index == 2)
                {
                    throw std::runtime_error("first input failure");
                }
                if (index == 6)
                {
                    throw std::runtime_error("later input failure");
                }
            });
    }

    try
    {
        jobs.runBatchAndWait(std::move(batch));
        FAIL() << "Expected a batch job error";
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_STREQ(error.what(), "first input failure");
    }

    EXPECT_EQ(finished.load(std::memory_order_relaxed), 9);
    EXPECT_EQ(jobs.queuedTaskCount(), 0u);
    EXPECT_NO_THROW(jobs.runAndWait([&]
        {
            finished.fetch_add(1, std::memory_order_relaxed);
        }));
    EXPECT_EQ(finished.load(std::memory_order_relaxed), 10);
}

TEST_P(JobSystemRunTests, BatchesOwnMoveOnlyCapturesAndReleaseThemBeforeReturning)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 2, false});
    std::atomic<int> released{0};
    std::atomic<int> sum{0};
    const auto deleter = [&](int *value)
    {
        delete value;
        released.fetch_add(1, std::memory_order_relaxed);
    };
    const auto makeWork = [&](int value)
    {
        return [owned = std::unique_ptr<int, decltype(deleter)>(new int(value), deleter), &sum]
        {
            sum.fetch_add(*owned, std::memory_order_relaxed);
        };
    };
    std::vector<decltype(makeWork(0))> batch;
    for (int value = 1; value <= 7; ++value)
    {
        batch.emplace_back(makeWork(value));
    }

    jobs.runBatchAndWait(std::move(batch));

    EXPECT_EQ(sum.load(std::memory_order_relaxed), 28);
    EXPECT_EQ(released.load(std::memory_order_relaxed), 7);
}

TEST_P(JobSystemRunTests, AcceptedJobsCanRunNestedBatchesAfterCloseWithACapacityOneQueue)
{
    JobSystem jobs(JobSystemConfig{GetParam(), 1, false});
    std::atomic<int> finished{0};
    jobs.runAndWait([&]
        {
            jobs.close();
            std::vector<std::function<void(WorkerIndex)>> batch;
            for (int index = 0; index < 3; ++index)
            {
                batch.emplace_back([&, index](WorkerIndex worker)
                    {
                        EXPECT_EQ(jobs.currentWorkerIndex(), std::optional<WorkerIndex>{worker});
                        finished.fetch_add(1, std::memory_order_relaxed);
                        if (index == 0)
                        {
                            std::vector<std::function<void()>> nested(2, [&]
                                {
                                    finished.fetch_add(1, std::memory_order_relaxed);
                                });
                            jobs.runBatchAndWait(std::move(nested));
                        }
                    });
            }
            jobs.runBatchAndWait(std::move(batch));
        });

    EXPECT_EQ(finished.load(std::memory_order_relaxed), 5);
    const std::vector<std::function<void()>> rejected{[] {}};
    EXPECT_THROW(jobs.runBatchAndWait(rejected), std::runtime_error);
    EXPECT_NO_THROW(jobs.join());
    EXPECT_LE(jobs.peakQueuedTaskCount(), 1u);
    EXPECT_EQ(jobs.queuedTaskCount(), 0u);
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

TEST(JobSystemTests, BatchJobsOverlapAndJoinWaitsForTheirCompletion)
{
    using namespace std::chrono_literals;
    JobSystem jobs(JobSystemConfig{2, 1, false});
    std::promise<void> firstStarted;
    std::promise<void> secondStarted;
    auto firstReady = firstStarted.get_future();
    auto secondReady = secondStarted.get_future();
    std::promise<void> release;
    const auto released = release.get_future().share();
    std::atomic<int> finished{0};
    std::thread::id firstThread;
    std::thread::id secondThread;
    std::vector<std::function<void()>> batch{
        [&]
        {
            firstThread = std::this_thread::get_id();
            firstStarted.set_value();
            if (released.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release first batch job");
            }
            finished.fetch_add(1, std::memory_order_relaxed);
        },
        [&]
        {
            secondThread = std::this_thread::get_id();
            secondStarted.set_value();
            if (released.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release second batch job");
            }
            finished.fetch_add(1, std::memory_order_relaxed);
        }};
    auto call = std::async(std::launch::async, [&]
        {
            jobs.runBatchAndWait(std::move(batch));
        });

    EXPECT_EQ(firstReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(secondReady.wait_for(2s), std::future_status::ready);
    auto joined = std::async(std::launch::async, [&]
        {
            jobs.join();
        });
    EXPECT_EQ(call.wait_for(20ms), std::future_status::timeout);
    EXPECT_EQ(joined.wait_for(20ms), std::future_status::timeout);
    release.set_value();
    EXPECT_EQ(call.wait_for(5s), std::future_status::ready);
    EXPECT_EQ(joined.wait_for(5s), std::future_status::ready);
    EXPECT_NO_THROW(call.get());
    EXPECT_NO_THROW(joined.get());

    EXPECT_NE(firstThread, secondThread);
    EXPECT_EQ(finished.load(std::memory_order_relaxed), 2);
    EXPECT_FALSE(jobs.isOpen());
    EXPECT_EQ(jobs.queuedTaskCount(), 0u);
}

TEST(JobSystemTests, BatchErrorsUseInputOrderAfterAllJobsFinishRatherThanCompletionOrder)
{
    using namespace std::chrono_literals;
    JobSystem jobs(JobSystemConfig{2, 2, false});
    std::promise<void> firstStarted;
    std::promise<void> laterFailed;
    auto firstReady = firstStarted.get_future();
    auto laterReady = laterFailed.get_future();
    std::promise<void> releaseFirst;
    const auto released = releaseFirst.get_future().share();
    std::atomic<int> finished{0};
    std::vector<std::function<void()>> batch{
        [&]
        {
            firstStarted.set_value();
            if (released.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release first batch job");
            }
            finished.fetch_add(1, std::memory_order_relaxed);
            throw std::runtime_error("first input failure");
        },
        [&]
        {
            finished.fetch_add(1, std::memory_order_relaxed);
            throw std::runtime_error("second input failure");
        },
        [&]
        {
            finished.fetch_add(1, std::memory_order_relaxed);
            laterFailed.set_value();
        }};
    auto call = std::async(std::launch::async, [&]
        {
            jobs.runBatchAndWait(std::move(batch));
        });

    EXPECT_EQ(firstReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(laterReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(call.wait_for(20ms), std::future_status::timeout);
    releaseFirst.set_value();
    EXPECT_EQ(call.wait_for(5s), std::future_status::ready);
    try
    {
        call.get();
        FAIL() << "Expected the first input error";
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_STREQ(error.what(), "first input failure");
    }
    EXPECT_EQ(finished.load(std::memory_order_relaxed), 3);
}

TEST(JobSystemTests, BatchFailureWaitsForLaterAcceptedJobsAndTheirCaptureRelease)
{
    using namespace std::chrono_literals;
    JobSystem jobs(JobSystemConfig{2, 2, false});
    std::promise<void> laterStarted;
    const auto laterReady = laterStarted.get_future().share();
    std::promise<void> firstFailureProcessed;
    auto failureReady = firstFailureProcessed.get_future();
    std::promise<void> releaseLater;
    const auto released = releaseLater.get_future().share();
    std::atomic<bool> laterFinished{false};
    std::atomic<bool> captureReleased{false};
    std::atomic<int> observedValue{0};
    auto owned = std::shared_ptr<int>(new int(42), [&](int *value)
        {
            delete value;
            captureReleased.store(true, std::memory_order_relaxed);
        });
    std::vector<std::function<void()>> batch;
    batch.emplace_back([&]
        {
            if (laterReady.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("later batch job did not start");
            }
            throw std::runtime_error("first input failure");
        });
    batch.emplace_back([&, owned = std::move(owned)]
        {
            laterStarted.set_value();
            if (released.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release later batch job");
            }
            observedValue.store(*owned, std::memory_order_relaxed);
            laterFinished.store(true, std::memory_order_relaxed);
        });
    batch.emplace_back([&]
        {
            // The other worker is blocked, so the first job has already failed.
            firstFailureProcessed.set_value();
        });
    auto call = std::async(std::launch::async, [&]
        {
            jobs.runBatchAndWait(std::move(batch));
        });

    EXPECT_EQ(laterReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(failureReady.wait_for(2s), std::future_status::ready);
    EXPECT_EQ(call.wait_for(20ms), std::future_status::timeout);
    EXPECT_FALSE(laterFinished.load(std::memory_order_relaxed));
    EXPECT_FALSE(captureReleased.load(std::memory_order_relaxed));
    releaseLater.set_value();
    EXPECT_EQ(call.wait_for(5s), std::future_status::ready);
    try
    {
        call.get();
        ADD_FAILURE() << "Expected the first input error";
    }
    catch (const std::runtime_error &error)
    {
        EXPECT_STREQ(error.what(), "first input failure");
    }
    EXPECT_TRUE(laterFinished.load(std::memory_order_relaxed));
    EXPECT_EQ(observedValue.load(std::memory_order_relaxed), 42);
    EXPECT_TRUE(captureReleased.load(std::memory_order_relaxed));
    EXPECT_NO_THROW(jobs.join());
}

TEST(JobSystemTests, PartialBatchAdmissionDrainsAcceptedJobsBeforeReportingSubmissionFailure)
{
    using namespace std::chrono_literals;
    JobSystem jobs(JobSystemConfig{1, 1, false});
    std::promise<void> firstStarted;
    auto firstReady = firstStarted.get_future();
    std::promise<void> releaseFirst;
    const auto released = releaseFirst.get_future().share();
    std::atomic<int> firstFinished{0};
    std::atomic<int> secondFinished{0};
    std::atomic<int> thirdFinished{0};
    std::vector<std::function<void()>> batch{
        [&]
        {
            firstStarted.set_value();
            if (released.wait_for(5s) != std::future_status::ready)
            {
                throw std::logic_error("test did not release accepted job");
            }
            firstFinished.fetch_add(1, std::memory_order_relaxed);
            throw std::logic_error("accepted job failure must not hide submission failure");
        },
        [&]
        {
            secondFinished.fetch_add(1, std::memory_order_relaxed);
        },
        [&]
        {
            thirdFinished.fetch_add(1, std::memory_order_relaxed);
        }};
    auto call = std::async(std::launch::async, [&]
        {
            jobs.runBatchAndWait(std::move(batch));
        });
    EXPECT_EQ(firstReady.wait_for(2s), std::future_status::ready);
    const auto deadline = std::chrono::steady_clock::now() + 2s;
    while (jobs.queuedTaskCount() != 1u && std::chrono::steady_clock::now() < deadline)
    {
        std::this_thread::yield();
    }
    EXPECT_EQ(jobs.queuedTaskCount(), 1u);
    jobs.close();
    EXPECT_EQ(call.wait_for(20ms), std::future_status::timeout);
    releaseFirst.set_value();
    EXPECT_EQ(call.wait_for(5s), std::future_status::ready);
    EXPECT_THROW(call.get(), std::runtime_error);

    EXPECT_EQ(firstFinished.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(secondFinished.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(thirdFinished.load(std::memory_order_relaxed), 0);
    EXPECT_EQ(jobs.peakQueuedTaskCount(), 1u);
    EXPECT_EQ(jobs.queuedTaskCount(), 0u);
    EXPECT_NO_THROW(jobs.join());
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
