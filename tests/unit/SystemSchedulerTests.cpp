#include "core/Scene.h"
#include "core/SystemRegistry.h"
#include "core/SystemScheduler.h"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <future>
#include <memory>
#include <stdexcept>
#include <string>
#include <thread>

#include <gtest/gtest.h>

namespace
{
using namespace std::chrono_literals;

class SystemSchedulerModes : public ::testing::TestWithParam<std::size_t>
{
protected:
    entt::registry registry;
    Physics2D physics{glm::vec2{0.0f, -9.8f}};
    FrameContext frame;
    SystemContext context{registry, physics, frame};
};

struct PublishedValue
{
    int value = 0;
};

struct UpdateState
{
    std::atomic<int> finished{0};
    int calls = 0;
    bool failUpdate = false;
    bool waitForRelease = false;
    std::shared_future<void> release;
    std::promise<void> started;
    std::thread::id updateThread;
    const FrameContext *observedFrame = nullptr;
    float observedDeltaTime = 0.0f;
    double observedElapsedTime = 0.0;
    KeyType observedKey = KeyType::None;
    bool observedEditMode = false;
};

class ProbeSystem final : public ISystem
{
public:
    explicit ProbeSystem(UpdateState &state, SystemThreadAffinity affinity = SystemThreadAffinity::AnyWorker)
        : m_state(state), m_affinity(affinity)
    {
    }

    void update(SystemContext &context) override
    {
        ++m_state.calls;
        m_state.updateThread = std::this_thread::get_id();
        if (m_state.waitForRelease)
        {
            m_state.started.set_value();
            if (m_state.release.wait_for(5s) != std::future_status::ready)
            {
                throw std::runtime_error("test did not release update");
            }
        }
        m_state.observedFrame = &context.frame;
        m_state.observedDeltaTime = context.frame.deltaTime;
        m_state.observedElapsedTime = context.frame.elapsedTime;
        m_state.observedEditMode = context.editMode;
        if (!context.frame.input.events.empty())
        {
            m_state.observedKey = context.frame.input.events.front().keyType;
        }
        m_state.finished.fetch_add(1, std::memory_order_relaxed);
        if (m_state.failUpdate)
        {
            throw std::runtime_error("update failed");
        }
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Probe").withAffinity(m_affinity);
    }

private:
    UpdateState &m_state;
    SystemThreadAffinity m_affinity;
};

struct SceneUpdateState : UpdateState
{
    int consumerCalls = 0;
    int observedValue = 0;
    int observedFinished = 0;
    entt::entity entity = entt::null;
    std::thread::id consumerThread;
};

std::shared_ptr<SceneUpdateState> sceneUpdateState;

class ScheduledProducer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        const auto state = sceneUpdateState;
        ProbeSystem update{*state};
        update.update(context);
        state->entity = context.registry.create();
        context.registry.emplace<PublishedValue>(state->entity).value = 42;
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Producer")
            .writes<PublishedValue>()
            .requestsStructuralChanges()
            .before("SystemSchedulerTests.Consumer");
    }
};

class ScheduledConsumer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        const auto state = sceneUpdateState;
        ++state->consumerCalls;
        state->consumerThread = std::this_thread::get_id();
        state->observedValue = context.registry.get<PublishedValue>(state->entity).value;
        state->observedFinished = state->finished.load(std::memory_order_relaxed);
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Consumer")
            .reads<PublishedValue>()
            .onMainThread();
    }
};

class ScheduledDestroyer final : public ISystem
{
public:
    void update(SystemContext &context) override
    {
        const auto state = sceneUpdateState;
        context.registry.destroy(state->entity);
        state->finished.fetch_add(1, std::memory_order_relaxed);
        if (state->failUpdate)
        {
            throw std::runtime_error("destroying update failed");
        }
    }

    SystemDescriptor descriptor() const override
    {
        return SystemDescriptor::named("SystemSchedulerTests.Destroyer").requestsStructuralChanges();
    }
};

void addScheduledSystems(Scene &scene)
{
    auto &registry = SystemRegistry::instance();
    registry.registerSystem<ScheduledProducer>("SystemSchedulerTests.Producer");
    registry.registerSystem<ScheduledConsumer>("SystemSchedulerTests.Consumer");
    scene.addSystem("SystemSchedulerTests.Consumer");
    scene.addSystem("SystemSchedulerTests.Producer");
    scene.setAllSystemsEnabled(true);
}

void expectWorkerThread(std::size_t workerCount, std::thread::id updateThread, std::thread::id caller)
{
    if (workerCount == 0)
    {
        EXPECT_EQ(updateThread, caller);
    }
    else
    {
        EXPECT_NE(updateThread, caller);
    }
}
} // namespace

TEST_P(SystemSchedulerModes, WorkerEligibleUpdatesRunOnWorkersWithAnInlineFallback)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    UpdateState state;
    ProbeSystem system{state};
    frame.deltaTime = 1.0f / 60.0f;
    frame.elapsedTime = 12.5;
    Event event;
    event.type = EventType::KeyPress;
    event.keyType = KeyType::W;
    frame.input.events.push_back(event);
    context.editMode = true;

    scheduler.run(system, context);

    EXPECT_EQ(state.calls, 1);
    EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);
    expectWorkerThread(GetParam(), state.updateThread, std::this_thread::get_id());
    EXPECT_EQ(state.observedFrame, &frame);
    EXPECT_FLOAT_EQ(state.observedDeltaTime, frame.deltaTime);
    EXPECT_DOUBLE_EQ(state.observedElapsedTime, frame.elapsedTime);
    EXPECT_EQ(state.observedKey, KeyType::W);
    EXPECT_TRUE(state.observedEditMode);
}

TEST_P(SystemSchedulerModes, MainThreadUpdatesRemainOnTheCaller)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    UpdateState state;
    ProbeSystem system{state, SystemThreadAffinity::MainThread};

    scheduler.run(system, context);

    EXPECT_EQ(state.calls, 1);
    EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);
    EXPECT_EQ(state.updateThread, std::this_thread::get_id());
}

TEST_P(SystemSchedulerModes, UpdateErrorsPropagateAndTheSchedulerCanRunAgain)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    for (const auto affinity : {SystemThreadAffinity::AnyWorker, SystemThreadAffinity::MainThread})
    {
        SCOPED_TRACE(affinity == SystemThreadAffinity::MainThread ? "main thread" : "worker");
        UpdateState state;
        ProbeSystem system{state, affinity};
        state.failUpdate = true;

        try
        {
            scheduler.run(system, context);
            FAIL() << "Expected the update error";
        }
        catch (const std::runtime_error &error)
        {
            EXPECT_STREQ(error.what(), "update failed");
        }
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);

        state.failUpdate = false;
        EXPECT_NO_THROW(scheduler.run(system, context));
        EXPECT_EQ(state.calls, 2);
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 2);
    }
}

TEST_P(SystemSchedulerModes, RunWaitsForTheWholeUpdateBeforeReturningOrThrowing)
{
    for (bool failUpdate : {false, true})
    {
        SCOPED_TRACE(failUpdate);
        SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
        UpdateState state;
        ProbeSystem system{state};
        state.waitForRelease = true;
        state.failUpdate = failUpdate;
        std::promise<void> release;
        state.release = release.get_future().share();
        auto started = state.started.get_future();
        frame.deltaTime = 1.0f / 60.0f;
        frame.elapsedTime = 7.0;

        auto run = std::async(std::launch::async, [&]
            {
                scheduler.run(system, context);
            });

        EXPECT_EQ(started.wait_for(5s), std::future_status::ready);
        EXPECT_EQ(run.wait_for(50ms), std::future_status::timeout);
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 0);
        release.set_value();
        EXPECT_EQ(run.wait_for(5s), std::future_status::ready);
        if (failUpdate)
        {
            EXPECT_THROW(run.get(), std::runtime_error);
        }
        else
        {
            EXPECT_NO_THROW(run.get());
        }
        EXPECT_EQ(state.finished.load(std::memory_order_relaxed), 1);
        EXPECT_EQ(state.observedFrame, &frame);
        EXPECT_FLOAT_EQ(state.observedDeltaTime, frame.deltaTime);
        EXPECT_DOUBLE_EQ(state.observedElapsedTime, frame.elapsedTime);
    }
}

TEST_P(SystemSchedulerModes, SceneConsumersObserveCompletedUpdatesInGraphOrder)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    Scene scene{"scheduled scene"};
    addScheduledSystems(scene);
    frame.deltaTime = 1.0f / 60.0f;

    scene.update(frame, scheduler, true);

    EXPECT_EQ(sceneUpdateState->calls, 1);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
    EXPECT_EQ(sceneUpdateState->observedFinished, 1);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 1);
    EXPECT_FLOAT_EQ(sceneUpdateState->observedDeltaTime, frame.deltaTime);
    expectWorkerThread(GetParam(), sceneUpdateState->updateThread, std::this_thread::get_id());
    EXPECT_EQ(sceneUpdateState->consumerThread, std::this_thread::get_id());
}

TEST(SystemSchedulerTests, StandaloneSceneUpdatesRunInline)
{
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    Scene scene{"standalone scheduled scene"};
    addScheduledSystems(scene);

    scene.update(FrameContext{}, true);

    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
    EXPECT_EQ(sceneUpdateState->observedFinished, 1);
    EXPECT_EQ(sceneUpdateState->updateThread, std::this_thread::get_id());
}

TEST(SystemSchedulerTests, SceneKeepsContextAliveAndIncludesTheFullUpdateInSystemTiming)
{
    SystemScheduler scheduler{JobSystemConfig{1, 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    const auto state = sceneUpdateState;
    state->waitForRelease = true;
    std::promise<void> release;
    state->release = release.get_future().share();
    auto started = state->started.get_future();
    Scene scene{"blocked system scene"};
    addScheduledSystems(scene);
    FrameContext frame;
    frame.deltaTime = 1.0f / 60.0f;

    auto update = std::async(std::launch::async, [&]
        {
            scene.update(frame, scheduler, true);
        });
    EXPECT_EQ(started.wait_for(5s), std::future_status::ready);
    const auto blockedSince = std::chrono::steady_clock::now();
    EXPECT_EQ(update.wait_for(50ms), std::future_status::timeout);
    const auto blockedTime = std::chrono::steady_clock::now() - blockedSince;
    release.set_value();
    EXPECT_EQ(update.wait_for(5s), std::future_status::ready);
    EXPECT_NO_THROW(update.get());

    EXPECT_EQ(state->observedValue, 42);
    EXPECT_FLOAT_EQ(state->observedDeltaTime, frame.deltaTime);
    const auto info = scene.getSystemInfo();
    ASSERT_EQ(info.size(), 2u);
    EXPECT_EQ(info.front().name, "SystemSchedulerTests.Producer");
    const double blockedMilliseconds = std::chrono::duration<double, std::milli>(blockedTime).count();
    EXPECT_GE(info.front().lastUpdateMilliseconds, blockedMilliseconds);
}

TEST_P(SystemSchedulerModes, SceneUpdateFailureSkipsDependentsAndAllowsAnotherUpdate)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    sceneUpdateState->failUpdate = true;
    Scene scene{"failing scheduled scene"};
    addScheduledSystems(scene);

    EXPECT_THROW(scene.update(FrameContext{}, scheduler, true), std::runtime_error);
    EXPECT_EQ(sceneUpdateState->calls, 1);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 0);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 1);

    sceneUpdateState->failUpdate = false;
    scene.update(FrameContext{}, scheduler, true);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
    EXPECT_EQ(sceneUpdateState->observedFinished, 2);
}

TEST_P(SystemSchedulerModes, ScenePauseAndDisablementDoNotLaunchSystemJobs)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    sceneUpdateState = std::make_shared<SceneUpdateState>();
    Scene scene{"disabled scheduled scene"};
    addScheduledSystems(scene);

    scene.update(FrameContext{}, scheduler);
    EXPECT_EQ(sceneUpdateState->calls, 0);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 0);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 0);

    scene.setAllSystemsEnabled(false);
    scene.update(FrameContext{}, scheduler, true);
    EXPECT_EQ(sceneUpdateState->calls, 0);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 0);
    EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 0);

    scene.setAllSystemsEnabled(true);
    scene.play();
    scene.update(FrameContext{}, scheduler);
    EXPECT_EQ(sceneUpdateState->consumerCalls, 1);
    EXPECT_EQ(sceneUpdateState->observedValue, 42);
}

TEST_P(SystemSchedulerModes, SceneCleansUpObjectsDestroyedByUpdatesBeforeReturningOrThrowing)
{
    SystemScheduler scheduler{JobSystemConfig{GetParam(), 8, true}};
    SystemRegistry::instance().registerSystem<ScheduledDestroyer>("SystemSchedulerTests.Destroyer");
    for (bool failUpdate : {false, true})
    {
        SCOPED_TRACE(failUpdate);
        sceneUpdateState = std::make_shared<SceneUpdateState>();
        sceneUpdateState->failUpdate = failUpdate;
        Scene scene{"system destruction scene"};
        sceneUpdateState->entity = scene.addGameObject(8, 8, "removed by system").getEntityId();
        scene.addSystem("SystemSchedulerTests.Destroyer");
        scene.setAllSystemsEnabled(true);

        if (failUpdate)
        {
            EXPECT_THROW(scene.update(FrameContext{}, scheduler, true), std::runtime_error);
        }
        else
        {
            EXPECT_NO_THROW(scene.update(FrameContext{}, scheduler, true));
        }

        EXPECT_EQ(sceneUpdateState->finished.load(std::memory_order_relaxed), 1);
        EXPECT_TRUE(scene.getGameObjects().empty());
    }
}

INSTANTIATE_TEST_SUITE_P(WorkerCounts, SystemSchedulerModes, ::testing::Values(0u, 1u, 2u));
