#include "core/EventHandler.h"
#include "core/InputSnapshot.h"

#include <gtest/gtest.h>

namespace
{
class InputSnapshotTests : public ::testing::Test
{
protected:
    void SetUp() override
    {
        resetLiveInput();
    }

    void TearDown() override
    {
        resetLiveInput();
    }

    static void key(int key, int action, int mods = 0)
    {
        EventHandler::glfwKeyCallBack(nullptr, key, 0, action, mods);
    }

    static void mouse(int button, int action)
    {
        EventHandler::glfwMouseButtonCallback(nullptr, button, action, 0);
    }

    static void cursor(double x, double y)
    {
        EventHandler::glfwCursorPosCallback(nullptr, x, y);
    }

    static void resetLiveInput()
    {
        auto *handler = EventHandler::instance();
        handler->setSceneViewportBounds({-1000000.0f, -1000000.0f}, {1000000.0f, 1000000.0f});
        cursor(0.0, 0.0);
        key(GLFW_KEY_W, GLFW_RELEASE);
        key(GLFW_KEY_A, GLFW_RELEASE);
        key(GLFW_KEY_SPACE, GLFW_RELEASE);
        mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
        mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
        mouse(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_RELEASE);
        handler->beginFrame();
    }
};
} // namespace

TEST_F(InputSnapshotTests, KeyboardEdgesSurvivePressReleaseAndRepeatWithinOneFrame)
{
    key(GLFW_KEY_W, GLFW_PRESS, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT);
    key(GLFW_KEY_W, GLFW_REPEAT);
    key(GLFW_KEY_W, GLFW_RELEASE);

    const InputSnapshot input = EventHandler::instance()->captureSnapshot();
    EXPECT_FALSE(input.isKeyHeld(KeyType::W));
    EXPECT_TRUE(input.wasKeyPressed(KeyType::W));
    EXPECT_TRUE(input.wasKeyReleased(KeyType::W));
    ASSERT_EQ(input.events.size(), 3u);
    EXPECT_EQ(input.events[0].type, EventType::Key);
    EXPECT_TRUE(input.events[0].ctrl);
    EXPECT_TRUE(input.events[0].shift);
    EXPECT_EQ(input.events[1].type, EventType::KeyRepeat);
    EXPECT_EQ(input.events[2].type, EventType::KeyRelease);

    EventHandler::instance()->beginFrame();
    const InputSnapshot nextFrame = EventHandler::instance()->captureSnapshot();
    EXPECT_FALSE(nextFrame.wasKeyPressed(KeyType::W));
    EXPECT_FALSE(nextFrame.wasKeyReleased(KeyType::W));
    EXPECT_FALSE(nextFrame.isKeyHeld(KeyType::W));
}

TEST_F(InputSnapshotTests, MouseEdgesAndLegacyPollingSurviveCapture)
{
    cursor(20.0, 30.0);
    EventHandler::instance()->beginFrame();
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);

    const InputSnapshot input = EventHandler::instance()->captureSnapshot();
    EXPECT_FALSE(input.isMouseButtonHeld(MouseButton::Left));
    EXPECT_TRUE(input.wasMouseButtonPressed(MouseButton::Left));
    EXPECT_TRUE(input.wasMouseButtonReleased(MouseButton::Left));
    EXPECT_TRUE(input.wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_TRUE(input.wasMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_EQ(input.getDragStart(), (glm::vec2{20.0f, 30.0f}));
    ASSERT_EQ(input.events.size(), 2u);
    EXPECT_EQ(input.events[0].type, EventType::MouseButtonPress);
    EXPECT_EQ(input.events[1].type, EventType::MouseButtonRelease);

    auto *handler = EventHandler::instance();
    EXPECT_TRUE(handler->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_TRUE(handler->wasMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_TRUE(handler->captureSnapshot().events.empty());
    handler->beginFrame();
    EXPECT_FALSE(handler->wasMouseButtonPressed(GLFW_MOUSE_BUTTON_LEFT));
    EXPECT_FALSE(handler->wasMouseButtonReleased(GLFW_MOUSE_BUTTON_LEFT));
}

TEST_F(InputSnapshotTests, AccumulatesFrameMouseAndScrollDeltas)
{
    cursor(100.0, 200.0);
    auto *handler = EventHandler::instance();
    handler->beginFrame();
    cursor(104.0, 195.0);
    cursor(110.0, 180.0);
    EventHandler::glfwScrollCallback(nullptr, 1.5, -2.0);
    EventHandler::glfwScrollCallback(nullptr, -0.5, 3.0);

    const InputSnapshot input = handler->captureSnapshot();
    EXPECT_EQ(input.getMousePos(), (glm::vec2{110.0f, 180.0f}));
    EXPECT_EQ(input.getMouseDelta(), (glm::vec2{10.0f, 20.0f}));
    EXPECT_EQ(input.getScrollDelta(), (glm::vec2{1.0f, 1.0f}));
    EXPECT_EQ(handler->getMouseDelta(), input.getMouseDelta());
    EXPECT_EQ(handler->getScrollDelta(), input.getScrollDelta());
    ASSERT_EQ(input.events.size(), 2u);
    EXPECT_EQ(input.events[0].type, EventType::MouseScroll);
    EXPECT_FALSE(input.events[0].isPressed);

    handler->beginFrame();
    const InputSnapshot nextFrame = handler->captureSnapshot();
    EXPECT_EQ(nextFrame.getMousePos(), input.getMousePos());
    EXPECT_EQ(nextFrame.getMouseDelta(), glm::vec2{0.0f});
    EXPECT_EQ(nextFrame.getScrollDelta(), glm::vec2{0.0f});
    EXPECT_TRUE(nextFrame.events.empty());
    EXPECT_EQ(input.getMouseDelta(), (glm::vec2{10.0f, 20.0f}));
    EXPECT_EQ(input.getScrollDelta(), (glm::vec2{1.0f, 1.0f}));
    EXPECT_EQ(input.events.size(), 2u);
}

TEST_F(InputSnapshotTests, SnapshotFreezesViewportAndBroadcastEventsForEveryReader)
{
    auto *handler = EventHandler::instance();
    handler->setSceneViewportBounds({0.0f, 0.0f}, {100.0f, 100.0f});
    cursor(50.0, 50.0);
    handler->beginFrame();
    key(GLFW_KEY_SPACE, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
    const InputSnapshot input = handler->captureSnapshot();

    const auto countKeyPresses = [](const InputSnapshot &snapshot)
    {
        std::size_t presses = 0;
        for (const Event &event : snapshot.events)
        {
            presses += event.type == EventType::Key ? 1u : 0u;
        }
        return presses;
    };
    EXPECT_EQ(countKeyPresses(input), 1u);
    EXPECT_EQ(countKeyPresses(input), 1u);

    key(GLFW_KEY_SPACE, GLFW_RELEASE);
    handler->setSceneViewportBounds({200.0f, 200.0f}, {300.0f, 300.0f});
    EXPECT_TRUE(input.isKeyHeld(KeyType::Space));
    EXPECT_TRUE(input.isMouseButtonHeld(MouseButton::Right));
    EXPECT_TRUE(input.isMouseInsideSceneViewport());
    EXPECT_FALSE(handler->isMouseButtonHeld(GLFW_MOUSE_BUTTON_RIGHT));
    const InputSnapshot laterCapture = handler->captureSnapshot();
    EXPECT_FALSE(laterCapture.isKeyHeld(KeyType::Space));
    EXPECT_FALSE(laterCapture.isMouseInsideSceneViewport());
    EXPECT_EQ(input.events.size(), 2u);
    ASSERT_EQ(laterCapture.events.size(), 1u);
    EXPECT_EQ(laterCapture.events[0].type, EventType::KeyRelease);
}

TEST_F(InputSnapshotTests, BeginFrameDiscardsUncapturedEventsAndPreservesHeldState)
{
    auto *handler = EventHandler::instance();
    key(GLFW_KEY_A, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_PRESS);
    EventHandler::glfwScrollCallback(nullptr, 0.0, 1.0);
    handler->beginFrame();

    const InputSnapshot input = handler->captureSnapshot();
    EXPECT_TRUE(input.isKeyHeld(KeyType::A));
    EXPECT_TRUE(input.isMouseButtonHeld(MouseButton::Middle));
    EXPECT_FALSE(input.wasKeyPressed(KeyType::A));
    EXPECT_FALSE(input.wasMouseButtonPressed(MouseButton::Middle));
    EXPECT_EQ(input.getScrollDelta(), glm::vec2{0.0f});
    EXPECT_TRUE(input.events.empty());
}

TEST_F(InputSnapshotTests, CapturesAllEventsInOrderBeyondTheFormerLimit)
{
    auto *handler = EventHandler::instance();
    key(GLFW_KEY_W, GLFW_PRESS);
    for (std::size_t index = 0; index < 4998; ++index)
    {
        key(GLFW_KEY_W, GLFW_REPEAT);
    }
    key(GLFW_KEY_W, GLFW_RELEASE);

    const InputSnapshot input = handler->captureSnapshot();
    ASSERT_EQ(input.events.size(), 5000u);
    EXPECT_EQ(input.events.front().type, EventType::Key);
    for (std::size_t index = 1; index + 1 < input.events.size(); ++index)
    {
        EXPECT_EQ(input.events[index].type, EventType::KeyRepeat);
    }
    EXPECT_EQ(input.events.back().type, EventType::KeyRelease);
    EXPECT_FALSE(input.isKeyHeld(KeyType::W));
    EXPECT_TRUE(input.wasKeyPressed(KeyType::W));
    EXPECT_TRUE(input.wasKeyReleased(KeyType::W));

    const InputSnapshot drained = handler->captureSnapshot();
    EXPECT_TRUE(drained.events.empty());
    EXPECT_TRUE(drained.wasKeyPressed(KeyType::W));
    EXPECT_TRUE(drained.wasKeyReleased(KeyType::W));
}

TEST_F(InputSnapshotTests, RuntimeFramesWithoutEventReadersDoNotAccumulateABacklog)
{
    auto *handler = EventHandler::instance();
    for (std::size_t frame = 0; frame < 512; ++frame)
    {
        handler->beginFrame();
        key(GLFW_KEY_W, GLFW_PRESS);
        key(GLFW_KEY_W, GLFW_RELEASE);
        EventHandler::glfwScrollCallback(nullptr, 0.0, 1.0);
        // Runtime captures input even when no editor or system reads events.
        handler->captureSnapshot();
    }

    handler->beginFrame();
    key(GLFW_KEY_SPACE, GLFW_PRESS);
    const InputSnapshot input = handler->captureSnapshot();
    ASSERT_EQ(input.events.size(), 1u);
    EXPECT_EQ(input.events[0].keyType, KeyType::Space);
    EXPECT_EQ(input.getScrollDelta(), glm::vec2{0.0f});
}

TEST_F(InputSnapshotTests, ReplayValuesDoNotDependOnLiveInput)
{
    InputSnapshot replay;
    replay.keyHeld[KeyType::F] = true;
    replay.keyPressed[KeyType::F] = true;
    replay.mouseHeld[static_cast<int>(MouseButton::Left)] = true;
    replay.mousePos = {12.0f, 34.0f};
    replay.events.push_back(Event{"FKey", EventType::Key, true, KeyType::F});

    const InputSnapshot frozen = replay;
    replay.keyHeld[KeyType::F] = false;
    replay.events.clear();
    EXPECT_TRUE(frozen.isKeyHeld(KeyType::F));
    EXPECT_TRUE(frozen.wasKeyPressed(KeyType::F));
    EXPECT_TRUE(frozen.isMouseButtonHeld(MouseButton::Left));
    EXPECT_EQ(frozen.getMousePos(), (glm::vec2{12.0f, 34.0f}));
    ASSERT_EQ(frozen.events.size(), 1u);
    EXPECT_EQ(frozen.events[0].keyType, KeyType::F);
}

TEST_F(InputSnapshotTests, AccumulatingFramesKeepsQuickPressReleaseAndLatestState)
{
    InputSnapshot pending;
    InputSnapshot press;
    press.keyHeld[KeyType::Space] = true;
    press.keyPressed[KeyType::Space] = true;
    press.mouseHeld[0] = true;
    press.mousePressed[0] = true;
    press.mouseDelta = {2.0f, 3.0f};
    press.scrollDelta = {1.0f, 0.5f};
    press.events.push_back(Event{"press", EventType::Key, true, KeyType::Space});
    pending.accumulate(press);

    InputSnapshot release;
    release.keyHeld[KeyType::Space] = false;
    release.keyPressed[KeyType::Space] = false; // A later frame must not erase the pending edge.
    release.keyReleased[KeyType::Space] = true;
    release.mouseHeld[0] = false;
    release.mousePressed[0] = false;
    release.mouseReleased[0] = true;
    release.mouseDelta = {-1.0f, 5.0f};
    release.scrollDelta = {2.0f, 0.25f};
    release.events.push_back(Event{"release", EventType::KeyRelease, false, KeyType::Space});
    pending.accumulate(release);

    InputSnapshot latest;
    latest.keyHeld[KeyType::D] = true;
    latest.mouseHeld[1] = true;
    latest.mousePos = {50.0f, 60.0f};
    latest.mouseDelta = {4.0f, -2.0f};
    latest.scrollDelta = {-0.5f, 1.0f};
    latest.dragStart = {10.0f, 20.0f};
    latest.hasSceneViewportBounds = true;
    latest.sceneViewportMin = {1.0f, 2.0f};
    latest.sceneViewportMax = {200.0f, 300.0f};
    pending.accumulate(latest);

    EXPECT_FALSE(pending.isKeyHeld(KeyType::Space));
    EXPECT_TRUE(pending.isKeyHeld(KeyType::D));
    EXPECT_TRUE(pending.wasKeyPressed(KeyType::Space));
    EXPECT_TRUE(pending.wasKeyReleased(KeyType::Space));
    EXPECT_FALSE(pending.isMouseButtonHeld(MouseButton::Left));
    EXPECT_TRUE(pending.isMouseButtonHeld(MouseButton::Right));
    EXPECT_TRUE(pending.wasMouseButtonPressed(MouseButton::Left));
    EXPECT_TRUE(pending.wasMouseButtonReleased(MouseButton::Left));
    EXPECT_EQ(pending.getMousePos(), (glm::vec2{50.0f, 60.0f}));
    EXPECT_EQ(pending.getMouseDelta(), (glm::vec2{5.0f, 6.0f}));
    EXPECT_EQ(pending.getScrollDelta(), (glm::vec2{2.5f, 1.75f}));
    EXPECT_EQ(pending.getDragStart(), (glm::vec2{10.0f, 20.0f}));
    EXPECT_EQ(pending.sceneViewportMin, latest.sceneViewportMin);
    EXPECT_EQ(pending.sceneViewportMax, latest.sceneViewportMax);
    EXPECT_TRUE(pending.hasSceneViewportBounds);
    ASSERT_EQ(pending.events.size(), 2u);
    EXPECT_EQ(pending.events[0].name, "press");
    EXPECT_EQ(pending.events[1].name, "release");
    EXPECT_TRUE(press.wasKeyPressed(KeyType::Space));
    ASSERT_EQ(press.events.size(), 1u);
}

TEST_F(InputSnapshotTests, ClearingConsumedInputPreservesHeldStateAndFrozenValues)
{
    InputSnapshot pending;
    pending.keyHeld[KeyType::D] = true;
    pending.keyPressed[KeyType::D] = true;
    pending.keyReleased[KeyType::A] = true;
    pending.mouseHeld[1] = true;
    pending.mousePressed[1] = true;
    pending.mouseReleased[0] = true;
    pending.mousePos = {50.0f, 60.0f};
    pending.mouseDelta = {4.0f, 5.0f};
    pending.scrollDelta = {1.0f, 2.0f};
    pending.dragStart = {10.0f, 20.0f};
    pending.hasSceneViewportBounds = true;
    pending.sceneViewportMin = {1.0f, 2.0f};
    pending.sceneViewportMax = {200.0f, 300.0f};
    pending.events.push_back(Event{"DKey", EventType::Key, true, KeyType::D});
    const InputSnapshot firstTick = pending;

    pending.clearTransientState();
    EXPECT_TRUE(pending.isKeyHeld(KeyType::D));
    EXPECT_TRUE(pending.isMouseButtonHeld(MouseButton::Right));
    EXPECT_FALSE(pending.wasKeyPressed(KeyType::D));
    EXPECT_FALSE(pending.wasKeyReleased(KeyType::A));
    EXPECT_FALSE(pending.wasMouseButtonPressed(MouseButton::Right));
    EXPECT_FALSE(pending.wasMouseButtonReleased(MouseButton::Left));
    EXPECT_EQ(pending.getMouseDelta(), glm::vec2{0.0f});
    EXPECT_EQ(pending.getScrollDelta(), glm::vec2{0.0f});
    EXPECT_TRUE(pending.events.empty());
    EXPECT_EQ(pending.getMousePos(), firstTick.getMousePos());
    EXPECT_EQ(pending.getDragStart(), firstTick.getDragStart());
    EXPECT_EQ(pending.sceneViewportMin, firstTick.sceneViewportMin);
    EXPECT_EQ(pending.sceneViewportMax, firstTick.sceneViewportMax);
    EXPECT_TRUE(pending.hasSceneViewportBounds);
    EXPECT_TRUE(firstTick.wasKeyPressed(KeyType::D));
    EXPECT_EQ(firstTick.events.size(), 1u);
}

TEST_F(InputSnapshotTests, AccumulatedFramesRetainEveryEventInOrderBeyondTheFormerLimit)
{
    InputSnapshot pending;
    InputSnapshot first;
    first.keyHeld[KeyType::F] = true;
    first.keyPressed[KeyType::F] = true;
    first.events.resize(5000, Event{"first", EventType::KeyRepeat, true, KeyType::F});
    first.events.front() = Event{"press", EventType::Key, true, KeyType::F};
    pending.accumulate(first);

    InputSnapshot second;
    second.keyHeld[KeyType::F] = true;
    second.events = {
        Event{"second0", EventType::KeyRepeat, true, KeyType::F},
        Event{"second1", EventType::KeyRepeat, true, KeyType::F},
        Event{"second2", EventType::KeyRepeat, true, KeyType::F},
        Event{"second3", EventType::KeyRepeat, true, KeyType::F},
        Event{"second4", EventType::KeyRepeat, true, KeyType::F},
    };
    pending.accumulate(second);

    InputSnapshot release;
    release.keyReleased[KeyType::F] = true;
    release.events.push_back(Event{"release", EventType::KeyRelease, false, KeyType::F});
    pending.accumulate(release);

    ASSERT_EQ(pending.events.size(), 5006u);
    EXPECT_EQ(pending.events.front().name, "press");
    EXPECT_EQ(pending.events[4999].name, "first");
    for (std::size_t index = 0; index < second.events.size(); ++index)
    {
        EXPECT_EQ(pending.events[5000 + index].name, second.events[index].name);
    }
    EXPECT_EQ(pending.events.back().name, "release");
    EXPECT_TRUE(pending.wasKeyPressed(KeyType::F));
    EXPECT_TRUE(pending.wasKeyReleased(KeyType::F));
    EXPECT_FALSE(pending.isKeyHeld(KeyType::F));

    pending.clearTransientState();
    pending.accumulate(release);
    ASSERT_EQ(pending.events.size(), 1u);
    EXPECT_EQ(pending.events.front().name, "release");
}
