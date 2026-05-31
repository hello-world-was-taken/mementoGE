#include "core/jobs/JobSystem.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <optional>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

TEST(JobSystemTests, ExecutesJobsAndWaitsForTheWholeGroup)
{
    JobSystem jobs(JobSystemConfig{2, 32, true});
    TaskGroup group = jobs.createGroup("basic");
    std::atomic<int> total{0};

    for (int value = 1; value <= 100; ++value)
    {
        jobs.submit(group,
            [&total, value]
            {
                total.fetch_add(value, std::memory_order_relaxed);
            });
    }

    jobs.wait(group);

    EXPECT_TRUE(group.isComplete());
    EXPECT_EQ(total.load(std::memory_order_relaxed), 5050);
}

TEST(JobSystemTests, ParallelForCoversEveryIndexExactlyOnce)
{
    JobSystem jobs(JobSystemConfig{3, 32, true});
    TaskGroup group = jobs.createGroup("parallel-for");
    constexpr std::size_t itemCount = 1003;
    std::vector<std::atomic<int>> visits(itemCount);
    std::atomic<bool> invalidWorkerIndex{false};

    jobs.parallelFor(group,
        itemCount,
        47,
        [&jobs, &visits, &invalidWorkerIndex](std::size_t begin, std::size_t end, WorkerIndex worker)
        {
            if (worker.value >= jobs.executionSlotCount())
            {
                invalidWorkerIndex.store(true, std::memory_order_relaxed);
            }
            for (std::size_t index = begin; index < end; ++index)
            {
                visits[index].fetch_add(1, std::memory_order_relaxed);
            }
        });

    jobs.wait(group);

    EXPECT_FALSE(invalidWorkerIndex.load(std::memory_order_relaxed));
    for (const auto &visitCount : visits)
    {
        EXPECT_EQ(visitCount.load(std::memory_order_relaxed), 1);
    }
}

TEST(JobSystemTests, SerialModeDefersTaskExceptionsUntilWait)
{
    JobSystem jobs(JobSystemConfig{0, 8, true});
    TaskGroup group = jobs.createGroup("serial-error");

    EXPECT_NO_THROW(jobs.submit(group,
        []
        {
            throw std::runtime_error("job failed");
        }));
    EXPECT_TRUE(group.isComplete());
    EXPECT_THROW(jobs.wait(group), std::runtime_error);
}

TEST(JobSystemTests, AWorkerWaitingForChildWorkHelpsExecuteIt)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    TaskGroup parent = jobs.createGroup("parent");
    std::atomic<bool> childRan{false};

    jobs.submit(parent,
        [&jobs, &childRan]
        {
            TaskGroup child = jobs.createGroup("child");
            jobs.submit(child,
                [&childRan]
                {
                    childRan.store(true, std::memory_order_release);
                });
            jobs.wait(child);
        });

    jobs.wait(parent);
    EXPECT_TRUE(childRan.load(std::memory_order_acquire));
}

TEST(JobSystemTests, WaitingOnTheCurrentJobsOwnGroupIsRejected)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    TaskGroup group = jobs.createGroup("self-wait");
    std::atomic<bool> rejected{false};

    jobs.submit(group,
        [&jobs, &group, &rejected]
        {
            try
            {
                jobs.wait(group);
            }
            catch (const std::logic_error &)
            {
                rejected.store(true, std::memory_order_release);
            }
        });

    jobs.wait(group);
    EXPECT_TRUE(rejected.load(std::memory_order_acquire));
}

TEST(JobSystemTests, WaitingOnASuspendedAncestorGroupIsRejected)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    TaskGroup parent = jobs.createGroup("ancestor");
    std::atomic<bool> rejected{false};

    jobs.submit(parent,
        [&jobs, &parent, &rejected]
        {
            TaskGroup child = jobs.createGroup("nested-child");
            jobs.submit(child,
                [&jobs, &parent, &rejected]
                {
                    try
                    {
                        jobs.wait(parent);
                    }
                    catch (const std::logic_error &)
                    {
                        rejected.store(true, std::memory_order_release);
                    }
                });
            jobs.wait(child);
        });

    jobs.wait(parent);
    EXPECT_TRUE(rejected.load(std::memory_order_acquire));
}

TEST(JobSystemTests, ReconstructedPoolRejectsGroupsFromItsPreviousLifetime)
{
    std::optional<JobSystem> jobs;
    jobs.emplace(JobSystemConfig{0, 8, true});
    TaskGroup oldGroup = jobs->createGroup("old-pool");

    jobs.reset();
    // Reuse the same object storage: address equality alone cannot distinguish
    // this new pool from the old group owner.
    jobs.emplace(JobSystemConfig{0, 8, true});

    EXPECT_THROW(jobs->submit(oldGroup,
                     []
                     {
                     }),
        std::invalid_argument);
    EXPECT_THROW(jobs->wait(oldGroup), std::invalid_argument);

    TaskGroup newGroup = jobs->createGroup("new-pool");
    EXPECT_NO_THROW(jobs->submit(newGroup,
        []
        {
        }));
    EXPECT_NO_THROW(jobs->wait(newGroup));
}

TEST(JobSystemTests, NestedPoolsRestoreExecutionContextAfterAnException)
{
    JobSystem outerJobs(JobSystemConfig{0, 8, true});
    JobSystem innerJobs(JobSystemConfig{0, 8, true});
    TaskGroup outer = outerJobs.createGroup("outer");

    outerJobs.submit(outer,
        [&](WorkerIndex outerWorker)
        {
            EXPECT_EQ(outerJobs.currentWorkerIndex(), std::optional<WorkerIndex>{outerWorker});
            TaskGroup inner = innerJobs.createGroup("inner");
            innerJobs.submit(inner,
                [&](WorkerIndex innerWorker)
                {
                    EXPECT_FALSE(outerJobs.currentWorkerIndex().has_value());
                    EXPECT_EQ(innerJobs.currentWorkerIndex(), std::optional<WorkerIndex>{innerWorker});
                    EXPECT_THROW(outerJobs.join(), std::logic_error);
                    throw std::runtime_error("inner failure");
                });
            EXPECT_THROW(innerJobs.wait(inner), std::runtime_error);
            EXPECT_EQ(outerJobs.currentWorkerIndex(), std::optional<WorkerIndex>{outerWorker});
            EXPECT_FALSE(innerJobs.currentWorkerIndex().has_value());
        });

    outerJobs.wait(outer);
    EXPECT_FALSE(outerJobs.currentWorkerIndex().has_value());
    EXPECT_FALSE(innerJobs.currentWorkerIndex().has_value());
}

TEST(JobSystemTests, SupportsMoveOnlyJobCaptures)
{
    JobSystem jobs(JobSystemConfig{1, 8, true});
    TaskGroup group = jobs.createGroup("move-only");
    std::atomic<int> result{0};

    jobs.submit(group,
        [value = std::make_unique<int>(42), &result]
        {
            result.store(*value, std::memory_order_release);
        });

    jobs.wait(group);
    EXPECT_EQ(result.load(std::memory_order_acquire), 42);
}

TEST(JobSystemTests, CloseRejectsRootWorkAndJoinDrainsAcceptedWork)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    TaskGroup group = jobs.createGroup("shutdown");
    std::promise<void> startedPromise;
    std::future<void> started = startedPromise.get_future();
    std::promise<void> releasePromise;
    std::shared_future<void> release = releasePromise.get_future().share();
    std::atomic<bool> finished{false};

    jobs.submit(group,
        [&startedPromise, release, &finished]
        {
            startedPromise.set_value();
            release.wait();
            finished.store(true, std::memory_order_release);
        });
    started.wait();

    jobs.close();
    EXPECT_FALSE(jobs.isOpen());
    EXPECT_THROW(jobs.submit(group,
                     []
                     {
                     }),
        std::runtime_error);

    releasePromise.set_value();
    jobs.join();
    jobs.wait(group);

    EXPECT_TRUE(finished.load(std::memory_order_acquire));
}

TEST(JobSystemTests, FatalShutdownFailurePrintsADiagnosticBeforeTerminating)
{
    EXPECT_DEATH(
        {
            auto jobs = std::make_unique<JobSystem>(JobSystemConfig{0, 8, true});
            TaskGroup group = jobs->createGroup("invalid-shutdown");
            jobs->submit(group,
                [&jobs]
                {
                    jobs.reset();
                });
        },
        "\\[FATAL\\] JobSystem: Shutdown failed: JobSystem::join cannot be called from one of its jobs");
}

TEST(JobSystemTests, RunningJobsCanSubmitRequiredChildrenAfterClose)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    TaskGroup parent = jobs.createGroup("parent-during-close");
    std::promise<void> startedPromise;
    std::future<void> started = startedPromise.get_future();
    std::promise<void> releasePromise;
    std::shared_future<void> release = releasePromise.get_future().share();
    std::atomic<bool> childRan{false};

    jobs.submit(parent,
        [&jobs, &startedPromise, release, &childRan]
        {
            startedPromise.set_value();
            release.wait();

            TaskGroup child = jobs.createGroup("child-during-close");
            jobs.submit(child,
                [&childRan]
                {
                    childRan.store(true, std::memory_order_release);
                });
            jobs.wait(child);
        });
    started.wait();

    jobs.close();
    releasePromise.set_value();
    jobs.join();
    jobs.wait(parent);

    EXPECT_TRUE(childRan.load(std::memory_order_acquire));
}

TEST(JobSystemTests, OneJobCanBelongToWorkAndLifetimeGroups)
{
    for (std::size_t workerCount : {0u, 2u})
    {
        SCOPED_TRACE(workerCount);
        JobSystem jobs(JobSystemConfig{workerCount, 8, true});
        TaskGroup work = jobs.createGroup("work");
        TaskGroup lifetime = jobs.createGroup("scene-lifetime");
        std::atomic<int> calls{0};

        jobs.submit(work,
            lifetime,
            [&calls]
            {
                calls.fetch_add(1, std::memory_order_relaxed);
            });

        jobs.wait(work);
        jobs.wait(lifetime);
        EXPECT_TRUE(work.isComplete());
        EXPECT_TRUE(lifetime.isComplete());
        EXPECT_EQ(calls.load(std::memory_order_relaxed), 1);
    }
}

TEST(JobSystemTests, ParallelForPublishesCompletionAndErrorsToBothGroups)
{
    for (std::size_t workerCount : {0u, 2u})
    {
        SCOPED_TRACE(workerCount);
        JobSystem jobs(JobSystemConfig{workerCount, 8, true});
        TaskGroup work = jobs.createGroup("parallel-work");
        TaskGroup lifetime = jobs.createGroup("parallel-lifetime");
        std::atomic<std::size_t> visited{0};

        jobs.parallelFor(work,
            lifetime,
            65,
            16,
            [&visited](std::size_t begin, std::size_t end)
            {
                visited.fetch_add(end - begin, std::memory_order_relaxed);
                if (begin == 32)
                {
                    throw std::runtime_error("range failed");
                }
            });

        EXPECT_THROW(jobs.wait(work), std::runtime_error);
        EXPECT_THROW(jobs.wait(lifetime), std::runtime_error);
        EXPECT_TRUE(work.isComplete());
        EXPECT_TRUE(lifetime.isComplete());
        EXPECT_EQ(visited.load(std::memory_order_relaxed), 65u);
    }
}

TEST(JobSystemTests, BothGroupsStayPendingUntilTheirJobCompletes)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    TaskGroup work = jobs.createGroup("pending-work");
    TaskGroup lifetime = jobs.createGroup("pending-lifetime");
    std::promise<void> startedPromise;
    std::future<void> started = startedPromise.get_future();
    std::promise<void> releasePromise;
    std::shared_future<void> release = releasePromise.get_future().share();

    jobs.submit(work,
        lifetime,
        [&startedPromise, release]
        {
            startedPromise.set_value();
            release.wait();
        });

    const std::future_status status = started.wait_for(std::chrono::seconds{5});
    EXPECT_EQ(status, std::future_status::ready);
    EXPECT_FALSE(work.isComplete());
    EXPECT_FALSE(lifetime.isComplete());
    releasePromise.set_value();

    jobs.wait(work);
    jobs.wait(lifetime);
    EXPECT_TRUE(work.isComplete());
    EXPECT_TRUE(lifetime.isComplete());
}

TEST(JobSystemTests, DuplicateGroupHandlesExecuteOneJobAndPreserveItsError)
{
    for (std::size_t workerCount : {0u, 2u})
    {
        SCOPED_TRACE(workerCount);
        JobSystem jobs(JobSystemConfig{workerCount, 8, true});
        TaskGroup group = jobs.createGroup("duplicate-membership");
        TaskGroup alias = group;
        std::atomic<int> calls{0};

        jobs.submit(group,
            alias,
            [&calls]
            {
                calls.fetch_add(1, std::memory_order_relaxed);
                throw std::runtime_error("duplicate group job failed");
            });

        EXPECT_THROW(jobs.wait(group), std::runtime_error);
        EXPECT_THROW(jobs.wait(alias), std::runtime_error);
        EXPECT_TRUE(group.isComplete());
        EXPECT_TRUE(alias.isComplete());
        EXPECT_EQ(calls.load(std::memory_order_relaxed), 1);
    }
}

TEST(JobSystemTests, BothGroupArgumentsAreValidatedBeforeJobAcceptance)
{
    for (std::size_t workerCount : {0u, 2u})
    {
        SCOPED_TRACE(workerCount);
        JobSystem jobs(JobSystemConfig{workerCount, 8, true});
        JobSystem otherJobs(JobSystemConfig{0, 8, true});
        TaskGroup valid = jobs.createGroup("valid-membership");
        TaskGroup invalid;
        TaskGroup foreign = otherJobs.createGroup("foreign-membership");
        std::atomic<int> calls{0};
        auto task = [&calls]
        {
            calls.fetch_add(1, std::memory_order_relaxed);
        };

        EXPECT_THROW(jobs.submit(invalid, valid, task), std::invalid_argument);
        EXPECT_TRUE(valid.isComplete());
        EXPECT_THROW(jobs.submit(valid, invalid, task), std::invalid_argument);
        EXPECT_TRUE(valid.isComplete());
        EXPECT_THROW(jobs.submit(foreign, valid, task), std::invalid_argument);
        EXPECT_TRUE(valid.isComplete());
        EXPECT_TRUE(foreign.isComplete());
        EXPECT_THROW(jobs.submit(valid, foreign, task), std::invalid_argument);
        EXPECT_TRUE(valid.isComplete());
        EXPECT_TRUE(foreign.isComplete());
        EXPECT_EQ(calls.load(std::memory_order_relaxed), 0);
        EXPECT_EQ(jobs.queuedTaskCount(), 0u);

        jobs.submit(valid, task);
        jobs.wait(valid);
        EXPECT_EQ(calls.load(std::memory_order_relaxed), 1);
    }
}

TEST(JobSystemTests, WaitingOnTheCurrentJobsSecondGroupIsRejected)
{
    for (std::size_t workerCount : {0u, 1u})
    {
        SCOPED_TRACE(workerCount);
        JobSystem jobs(JobSystemConfig{workerCount, 8, false});
        TaskGroup work = jobs.createGroup("self-wait-work");
        TaskGroup lifetime = jobs.createGroup("self-wait-lifetime");
        std::atomic<bool> rejected{false};

        jobs.submit(work,
            lifetime,
            [&jobs, &lifetime, &rejected]
            {
                try
                {
                    jobs.wait(lifetime);
                }
                catch (const std::logic_error &)
                {
                    rejected.store(true, std::memory_order_release);
                }
            });

        jobs.wait(work);
        jobs.wait(lifetime);
        EXPECT_TRUE(rejected.load(std::memory_order_acquire));
    }
}

TEST(JobSystemTests, WaitingOnASuspendedAncestorsSecondGroupIsRejected)
{
    for (std::size_t workerCount : {0u, 1u})
    {
        SCOPED_TRACE(workerCount);
        JobSystem jobs(JobSystemConfig{workerCount, 8, false});
        TaskGroup work = jobs.createGroup("ancestor-work");
        TaskGroup lifetime = jobs.createGroup("ancestor-lifetime");
        std::atomic<bool> rejected{false};

        jobs.submit(work,
            lifetime,
            [&jobs, &lifetime, &rejected]
            {
                TaskGroup child = jobs.createGroup("second-membership-child");
                jobs.submit(child,
                    [&jobs, &lifetime, &rejected]
                    {
                        try
                        {
                            jobs.wait(lifetime);
                        }
                        catch (const std::logic_error &)
                        {
                            rejected.store(true, std::memory_order_release);
                        }
                    });
                jobs.wait(child);
            });

        jobs.wait(work);
        jobs.wait(lifetime);
        EXPECT_TRUE(rejected.load(std::memory_order_acquire));
    }
}

TEST(JobSystemTests, QueuedJobsKeepBothGroupStatesAliveWithoutPublicHandles)
{
    JobSystem jobs(JobSystemConfig{1, 8, false});
    TaskGroup blocker = jobs.createGroup("handle-lifetime-blocker");
    std::promise<void> startedPromise;
    std::future<void> started = startedPromise.get_future();
    std::promise<void> releasePromise;
    std::shared_future<void> release = releasePromise.get_future().share();
    std::atomic<int> calls{0};

    jobs.submit(blocker,
        [&startedPromise, release]
        {
            startedPromise.set_value();
            release.wait();
        });
    EXPECT_EQ(started.wait_for(std::chrono::seconds{5}), std::future_status::ready);

    {
        TaskGroup work = jobs.createGroup("released-work-handle");
        TaskGroup lifetime = jobs.createGroup("released-lifetime-handle");
        jobs.submit(work,
            lifetime,
            [&calls]
            {
                calls.fetch_add(1, std::memory_order_relaxed);
            });
    }

    releasePromise.set_value();
    jobs.join();
    EXPECT_EQ(calls.load(std::memory_order_relaxed), 1);
}
