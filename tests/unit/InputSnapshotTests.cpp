#include "core/EventHandler.h"
#include "core/InputSnapshot.h"

#include <algorithm>
#include <gtest/gtest.h>
#include <utility>

namespace
{
std::size_t countEvents(const InputSnapshot &input, EventType type)
{
    return static_cast<std::size_t>(std::count_if(input.events.begin(), input.events.end(),
        [type](const Event &event) { return event.type == type; }));
}

const Event *findEvent(const InputSnapshot &input, EventType type)
{
    const auto found = std::find_if(input.events.begin(), input.events.end(),
        [type](const Event &event) { return event.type == type; });
    return found == input.events.end() ? nullptr : &*found;
}

class InputSnapshotTests : public ::testing::Test
{
protected:
    void SetUp() override { resetLiveInput(); }
    void TearDown() override { resetLiveInput(); }

    void key(int code, int action, int mods = 0, int scanCode = 0)
    {
        const std::pair<int, int> token{code, scanCode};
        const auto tracked = std::find(nativeKeys.begin(), nativeKeys.end(), token);
        if (action == GLFW_RELEASE)
        {
            if (tracked != nativeKeys.end()) nativeKeys.erase(tracked);
        }
        else if (action == GLFW_PRESS || action == GLFW_REPEAT)
        {
            if (tracked == nativeKeys.end()) nativeKeys.push_back(token);
        }
        EventHandler::glfwKeyCallBack(nullptr, code, scanCode, action, mods);
    }

    void mouse(int button, int action, int mods = 0)
    {
        const auto tracked = std::find(nativeButtons.begin(), nativeButtons.end(), button);
        if (action == GLFW_RELEASE)
        {
            if (tracked != nativeButtons.end()) nativeButtons.erase(tracked);
        }
        else if (action == GLFW_PRESS && tracked == nativeButtons.end())
        {
            nativeButtons.push_back(button);
        }
        EventHandler::glfwMouseButtonCallback(nullptr, button, action, mods);
    }

    static void cursor(double x, double y)
    {
        EventHandler::glfwCursorPosCallback(nullptr, x, y);
    }

    void resetLiveInput()
    {
        auto *handler = EventHandler::instance();
        // Keep native callback tokens in the fixture; public events deliberately
        // expose only engine keys and buttons.
        for (const auto &[code, scanCode] : nativeKeys)
        {
            EventHandler::glfwKeyCallBack(nullptr, code, scanCode, GLFW_RELEASE, 0);
        }
        for (int button : nativeButtons)
        {
            EventHandler::glfwMouseButtonCallback(nullptr, button, GLFW_RELEASE, 0);
        }
        nativeKeys.clear();
        nativeButtons.clear();
        EventHandler::glfwWindowFocusCallback(nullptr, GLFW_TRUE);
        handler->setSceneViewportBounds({-1000000.0f, -1000000.0f}, {1000000.0f, 1000000.0f});
        cursor(0.0, 0.0);
        handler->captureSnapshot();
    }

    std::vector<std::pair<int, int>> nativeKeys;
    std::vector<int> nativeButtons;
};
} // namespace

TEST_F(InputSnapshotTests, KeyboardCallbacksPreserveNativeOrderAndModifiers)
{
    key(GLFW_KEY_W, GLFW_PRESS, GLFW_MOD_CONTROL | GLFW_MOD_SHIFT, 17);
    key(GLFW_KEY_W, GLFW_REPEAT, 0, 17);
    key(GLFW_KEY_W, GLFW_RELEASE, 0, 17);

    const auto input = EventHandler::instance()->captureSnapshot();
    ASSERT_EQ(input.events.size(), 4u);
    EXPECT_EQ(input.events[0].type, EventType::KeyPress);
    EXPECT_EQ(input.events[0].keyType, KeyType::W);
    EXPECT_TRUE(input.events[0].ctrl);
    EXPECT_TRUE(input.events[0].shift);
    EXPECT_EQ(input.events[1].type, EventType::KeyRepeat);
    EXPECT_EQ(input.events[1].keyType, KeyType::W);
    EXPECT_EQ(input.events[2].type, EventType::KeyRelease);
    EXPECT_EQ(input.events[2].keyType, KeyType::W);
    EXPECT_EQ(input.events[3].type, EventType::MousePosition);
    EXPECT_EQ(countEvents(input, EventType::KeyHeld), 0u);
}

TEST_F(InputSnapshotTests, StandardKeysTranslateToEngineKeys)
{
    const std::pair<int, KeyType> keys[] = {
        {GLFW_KEY_B, KeyType::B},
        {GLFW_KEY_Q, KeyType::Q},
        {GLFW_KEY_0, KeyType::Digit0},
        {GLFW_KEY_9, KeyType::Digit9},
        {GLFW_KEY_APOSTROPHE, KeyType::Apostrophe},
        {GLFW_KEY_BACKSLASH, KeyType::Backslash},
        {GLFW_KEY_GRAVE_ACCENT, KeyType::GraveAccent},
        {GLFW_KEY_WORLD_1, KeyType::International1},
        {GLFW_KEY_WORLD_2, KeyType::International2},
        {GLFW_KEY_TAB, KeyType::Tab},
        {GLFW_KEY_BACKSPACE, KeyType::Backspace},
        {GLFW_KEY_HOME, KeyType::Home},
        {GLFW_KEY_PAGE_DOWN, KeyType::PageDown},
        {GLFW_KEY_F1, KeyType::F1},
        {GLFW_KEY_F12, KeyType::F12},
        {GLFW_KEY_F25, KeyType::F25},
        {GLFW_KEY_KP_0, KeyType::Keypad0},
        {GLFW_KEY_KP_9, KeyType::Keypad9},
        {GLFW_KEY_KP_ADD, KeyType::KeypadAdd},
        {GLFW_KEY_KP_ENTER, KeyType::KeypadEnter},
        {GLFW_KEY_KP_EQUAL, KeyType::KeypadEqual},
        {GLFW_KEY_LEFT_SHIFT, KeyType::LeftShift},
        {GLFW_KEY_RIGHT_SHIFT, KeyType::RightShift},
        {GLFW_KEY_LEFT_CONTROL, KeyType::LeftControl},
        {GLFW_KEY_RIGHT_CONTROL, KeyType::RightControl},
        {GLFW_KEY_LEFT_ALT, KeyType::LeftAlt},
        {GLFW_KEY_RIGHT_ALT, KeyType::RightAlt},
        {GLFW_KEY_LEFT_SUPER, KeyType::LeftSuper},
        {GLFW_KEY_RIGHT_SUPER, KeyType::RightSuper},
        {GLFW_KEY_MENU, KeyType::Menu},
    };
    for (const auto &[native, translated] : keys)
    {
        SCOPED_TRACE(native);
        key(native, GLFW_PRESS, GLFW_MOD_SUPER | GLFW_MOD_ALT, 87);
        const auto input = EventHandler::instance()->captureSnapshot();
        ASSERT_EQ(input.events.size(), 3u);
        EXPECT_EQ(input.events[0].type, EventType::KeyPress);
        EXPECT_EQ(input.events[0].keyType, translated);
        EXPECT_TRUE(input.events[0].cmd);
        EXPECT_TRUE(input.events[0].alt);
        EXPECT_EQ(input.events[1].type, EventType::KeyHeld);
        EXPECT_EQ(input.events[1].keyType, translated);
        key(native, GLFW_RELEASE, 0, 87);
        const auto released = EventHandler::instance()->captureSnapshot();
        ASSERT_EQ(released.events.size(), 2u);
        EXPECT_EQ(released.events[0].type, EventType::KeyRelease);
        EXPECT_EQ(released.events[0].keyType, translated);
    }
}

TEST_F(InputSnapshotTests, UnknownKeyTokensRemainDistinctByScancode)
{
    key(GLFW_KEY_UNKNOWN, GLFW_PRESS, 0, 120);
    key(GLFW_KEY_UNKNOWN, GLFW_PRESS, 0, 121);
    auto *handler = EventHandler::instance();
    const auto bothHeld = handler->captureSnapshot();
    EXPECT_EQ(countEvents(bothHeld, EventType::KeyHeld), 2u);
    for (const Event &event : bothHeld.events)
    {
        if (event.type == EventType::KeyPress || event.type == EventType::KeyHeld)
        {
            EXPECT_EQ(event.keyType, KeyType::Unknown);
        }
    }
    key(GLFW_KEY_UNKNOWN, GLFW_RELEASE, 0, 120);
    const auto input = handler->captureSnapshot();
    EXPECT_EQ(countEvents(input, EventType::KeyRelease), 1u);
    EXPECT_EQ(countEvents(input, EventType::KeyHeld), 1u);
    const Event *held = findEvent(input, EventType::KeyHeld);
    ASSERT_NE(held, nullptr);
    EXPECT_EQ(held->keyType, KeyType::Unknown);
    key(GLFW_KEY_UNKNOWN, GLFW_RELEASE, 0, 121);
    const auto released = handler->captureSnapshot();
    EXPECT_EQ(countEvents(released, EventType::KeyRelease), 1u);
    EXPECT_EQ(countEvents(released, EventType::KeyHeld), 0u);
}

TEST_F(InputSnapshotTests, MouseButtonsTranslateToEngineButtons)
{
    const std::pair<int, MouseButton> buttons[] = {
        {GLFW_MOUSE_BUTTON_LEFT, MouseButton::Left},
        {GLFW_MOUSE_BUTTON_RIGHT, MouseButton::Right},
        {GLFW_MOUSE_BUTTON_MIDDLE, MouseButton::Middle},
        {GLFW_MOUSE_BUTTON_4, MouseButton::Button4},
        {GLFW_MOUSE_BUTTON_5, MouseButton::Button5},
        {GLFW_MOUSE_BUTTON_6, MouseButton::Button6},
        {GLFW_MOUSE_BUTTON_7, MouseButton::Button7},
        {GLFW_MOUSE_BUTTON_8, MouseButton::Button8},
    };
    for (const auto &[native, translated] : buttons)
    {
        SCOPED_TRACE(native);
        mouse(native, GLFW_PRESS);
        const auto input = EventHandler::instance()->captureSnapshot();
        ASSERT_EQ(input.events.size(), 3u);
        EXPECT_EQ(input.events[0].type, EventType::MouseButtonPress);
        EXPECT_EQ(input.events[0].mouseButton, translated);
        EXPECT_EQ(input.events[1].type, EventType::MouseButtonHeld);
        EXPECT_EQ(input.events[1].mouseButton, translated);
        mouse(native, GLFW_RELEASE);
        const auto released = EventHandler::instance()->captureSnapshot();
        ASSERT_EQ(released.events.size(), 2u);
        EXPECT_EQ(released.events[0].type, EventType::MouseButtonRelease);
        EXPECT_EQ(released.events[0].mouseButton, translated);
    }
}

TEST_F(InputSnapshotTests, UnknownMouseButtonsKeepIndependentHeldState)
{
    constexpr int firstNativeButton = GLFW_MOUSE_BUTTON_LAST + 1;
    constexpr int secondNativeButton = GLFW_MOUSE_BUTTON_LAST + 2;
    mouse(firstNativeButton, GLFW_PRESS);
    mouse(secondNativeButton, GLFW_PRESS);
    auto *handler = EventHandler::instance();
    const auto bothHeld = handler->captureSnapshot();
    EXPECT_EQ(countEvents(bothHeld, EventType::MouseButtonHeld), 2u);
    for (const Event &event : bothHeld.events)
    {
        if (event.type == EventType::MouseButtonPress || event.type == EventType::MouseButtonHeld)
        {
            EXPECT_EQ(event.mouseButton, MouseButton::Unknown);
        }
    }

    mouse(firstNativeButton, GLFW_RELEASE);
    const auto oneHeld = handler->captureSnapshot();
    EXPECT_EQ(countEvents(oneHeld, EventType::MouseButtonRelease), 1u);
    EXPECT_EQ(countEvents(oneHeld, EventType::MouseButtonHeld), 1u);
    const Event *held = findEvent(oneHeld, EventType::MouseButtonHeld);
    ASSERT_NE(held, nullptr);
    EXPECT_EQ(held->mouseButton, MouseButton::Unknown);

    mouse(secondNativeButton, GLFW_RELEASE);
    const auto released = handler->captureSnapshot();
    EXPECT_EQ(countEvents(released, EventType::MouseButtonRelease), 1u);
    EXPECT_EQ(countEvents(released, EventType::MouseButtonHeld), 0u);
}

TEST_F(InputSnapshotTests, HeldInputIsEmittedEveryCaptureWithoutNativeRepeat)
{
    auto *handler = EventHandler::instance();
    key(GLFW_KEY_W, GLFW_PRESS);
    const auto first = handler->captureSnapshot();
    EXPECT_EQ(countEvents(first, EventType::KeyPress), 1u);
    EXPECT_EQ(countEvents(first, EventType::KeyHeld), 1u);

    const auto continued = handler->captureSnapshot();
    ASSERT_EQ(continued.events.size(), 2u);
    EXPECT_EQ(continued.events[0].type, EventType::KeyHeld);
    EXPECT_EQ(continued.events[0].keyType, KeyType::W);
    EXPECT_EQ(countEvents(continued, EventType::KeyRepeat), 0u);
    EXPECT_EQ(countEvents(continued, EventType::KeyPress), 0u);

    key(GLFW_KEY_W, GLFW_RELEASE);
    const auto release = handler->captureSnapshot();
    EXPECT_EQ(countEvents(release, EventType::KeyRelease), 1u);
    EXPECT_EQ(countEvents(release, EventType::KeyHeld), 0u);
}

TEST_F(InputSnapshotTests, InitialCursorPositionSeedsFirstClickAndScrollWithoutMovement)
{
    auto *handler = EventHandler::instance();
    handler->initializeCursorPosition(37.0, 59.0);
    const auto initial = handler->captureSnapshot();
    ASSERT_EQ(initial.events.size(), 1u);
    EXPECT_EQ(initial.events[0].type, EventType::MousePosition);
    EXPECT_FLOAT_EQ(initial.events[0].mouseX, 37.0f);
    EXPECT_FLOAT_EQ(initial.events[0].mouseY, 59.0f);

    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    EventHandler::glfwScrollCallback(nullptr, 2.0, -3.0);
    const auto input = handler->captureSnapshot();
    EXPECT_EQ(countEvents(input, EventType::MouseMove), 0u);
    for (EventType type : {EventType::MouseButtonPress, EventType::MouseButtonHeld, EventType::MouseScroll})
    {
        const Event *event = findEvent(input, type);
        ASSERT_NE(event, nullptr);
        EXPECT_FLOAT_EQ(event->mouseX, 37.0f);
        EXPECT_FLOAT_EQ(event->mouseY, 59.0f);
    }
    const Event *held = findEvent(input, EventType::MouseButtonHeld);
    ASSERT_NE(held, nullptr);
    EXPECT_FLOAT_EQ(held->dragStartX, 37.0f);
    EXPECT_FLOAT_EQ(held->dragStartY, 59.0f);

    cursor(40.0, 51.0);
    const auto moved = handler->captureSnapshot();
    const Event *movement = findEvent(moved, EventType::MouseMove);
    ASSERT_NE(movement, nullptr);
    EXPECT_FLOAT_EQ(movement->mouseDeltaX, 3.0f);
    EXPECT_FLOAT_EQ(movement->mouseDeltaY, 8.0f);
}

TEST_F(InputSnapshotTests, ModifierCallbacksRefreshAlreadyHeldKeysAndButtonsWithoutRepeat)
{
    auto *handler = EventHandler::instance();
    key(GLFW_KEY_W, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
    handler->captureSnapshot();

    const std::pair<int, int> modifiers[] = {
        {GLFW_KEY_LEFT_SHIFT, GLFW_MOD_SHIFT},
        {GLFW_KEY_RIGHT_CONTROL, GLFW_MOD_CONTROL},
        {GLFW_KEY_LEFT_ALT, GLFW_MOD_ALT},
        {GLFW_KEY_RIGHT_SUPER, GLFW_MOD_SUPER},
    };
    for (const auto &[nativeKey, mods] : modifiers)
    {
        SCOPED_TRACE(nativeKey);
        key(nativeKey, GLFW_PRESS, mods);
        cursor(static_cast<double>(nativeKey), 50.0);
        EventHandler::glfwScrollCallback(nullptr, 0.0, 1.0);
        const auto modified = handler->captureSnapshot();
        EXPECT_EQ(countEvents(modified, EventType::KeyRepeat), 0u);
        EXPECT_EQ(countEvents(modified, EventType::MouseMove), 1u);
        EXPECT_EQ(countEvents(modified, EventType::MouseScroll), 1u);
        std::size_t checked = 0;
        for (const Event &event : modified.events)
        {
            EXPECT_EQ(event.shift, (mods & GLFW_MOD_SHIFT) != 0);
            EXPECT_EQ(event.ctrl, (mods & GLFW_MOD_CONTROL) != 0);
            EXPECT_EQ(event.alt, (mods & GLFW_MOD_ALT) != 0);
            EXPECT_EQ(event.cmd, (mods & GLFW_MOD_SUPER) != 0);
            if ((event.type == EventType::KeyHeld && event.keyType == KeyType::W) ||
                event.type == EventType::MouseButtonHeld)
            {
                ++checked;
            }
        }
        EXPECT_EQ(checked, 2u);

        key(nativeKey, GLFW_RELEASE);
        const auto released = handler->captureSnapshot();
        EXPECT_EQ(countEvents(released, EventType::KeyRepeat), 0u);
        for (const Event &event : released.events)
        {
            EXPECT_FALSE(event.shift);
            EXPECT_FALSE(event.ctrl);
            EXPECT_FALSE(event.alt);
            EXPECT_FALSE(event.cmd);
        }
    }
}

TEST_F(InputSnapshotTests, FocusLossReleasesHeldInputsOnceAndFocusGainDoesNotRestoreThem)
{
    auto *handler = EventHandler::instance();
    handler->initializeCursorPosition(12.0, 18.0);
    key(GLFW_KEY_W, GLFW_PRESS, GLFW_MOD_SHIFT);
    key(GLFW_KEY_UNKNOWN, GLFW_PRESS, GLFW_MOD_SHIFT, 120);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS, GLFW_MOD_SHIFT);
    constexpr int unknownButton = GLFW_MOUSE_BUTTON_LAST + 1;
    mouse(unknownButton, GLFW_PRESS, GLFW_MOD_SHIFT);
    handler->captureSnapshot();
    cursor(30.0, 40.0);
    handler->captureSnapshot();

    EventHandler::glfwWindowFocusCallback(nullptr, GLFW_FALSE);
    const auto lostFocus = handler->captureSnapshot();
    ASSERT_EQ(lostFocus.events.size(), 5u);
    EXPECT_EQ(countEvents(lostFocus, EventType::KeyRelease), 2u);
    EXPECT_EQ(countEvents(lostFocus, EventType::MouseButtonRelease), 2u);
    EXPECT_EQ(countEvents(lostFocus, EventType::KeyHeld), 0u);
    EXPECT_EQ(countEvents(lostFocus, EventType::MouseButtonHeld), 0u);
    for (const Event &event : lostFocus.events)
    {
        EXPECT_FALSE(event.shift);
        EXPECT_FALSE(event.ctrl);
        EXPECT_FALSE(event.alt);
        EXPECT_FALSE(event.cmd);
        if (event.type == EventType::MouseButtonRelease && event.mouseButton == MouseButton::Left)
        {
            EXPECT_FLOAT_EQ(event.mouseX, 30.0f);
            EXPECT_FLOAT_EQ(event.mouseY, 40.0f);
            EXPECT_FLOAT_EQ(event.dragStartX, 12.0f);
            EXPECT_FLOAT_EQ(event.dragStartY, 18.0f);
        }
    }

    // GLFW follows focus loss with synthetic releases. They must not release
    // the same inputs a second time, including unknown native identities.
    key(GLFW_KEY_W, GLFW_RELEASE);
    key(GLFW_KEY_UNKNOWN, GLFW_RELEASE, 0, 120);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
    mouse(unknownButton, GLFW_RELEASE);
    EventHandler::glfwWindowFocusCallback(nullptr, GLFW_FALSE);
    const auto syntheticReleases = handler->captureSnapshot();
    ASSERT_EQ(syntheticReleases.events.size(), 1u);
    EXPECT_EQ(syntheticReleases.events[0].type, EventType::MousePosition);

    EventHandler::glfwWindowFocusCallback(nullptr, GLFW_TRUE);
    const auto regainedFocus = handler->captureSnapshot();
    ASSERT_EQ(regainedFocus.events.size(), 1u);
    EXPECT_EQ(regainedFocus.events[0].type, EventType::MousePosition);

    key(GLFW_KEY_W, GLFW_PRESS);
    const auto pressedAgain = handler->captureSnapshot();
    EXPECT_EQ(countEvents(pressedAgain, EventType::KeyPress), 1u);
    EXPECT_EQ(countEvents(pressedAgain, EventType::KeyHeld), 1u);
}

TEST_F(InputSnapshotTests, MouseMovesKeepIndividualPositionsAndUpwardDeltas)
{
    auto *handler = EventHandler::instance();
    cursor(100.0, 200.0);
    handler->captureSnapshot();
    cursor(104.0, 195.0);
    cursor(110.0, 180.0);
    const auto input = handler->captureSnapshot();
    ASSERT_EQ(input.events.size(), 3u);
    EXPECT_EQ(input.events[0].type, EventType::MouseMove);
    EXPECT_FLOAT_EQ(input.events[0].mouseX, 104.0f);
    EXPECT_FLOAT_EQ(input.events[0].mouseY, 195.0f);
    EXPECT_FLOAT_EQ(input.events[0].mouseDeltaX, 4.0f);
    EXPECT_FLOAT_EQ(input.events[0].mouseDeltaY, 5.0f);
    EXPECT_EQ(input.events[1].type, EventType::MouseMove);
    EXPECT_FLOAT_EQ(input.events[1].mouseDeltaX, 6.0f);
    EXPECT_FLOAT_EQ(input.events[1].mouseDeltaY, 15.0f);
    EXPECT_EQ(input.events[2].type, EventType::MousePosition);
    EXPECT_FLOAT_EQ(input.events[2].mouseX, 110.0f);
    EXPECT_FLOAT_EQ(input.events[2].mouseY, 180.0f);

    const auto next = handler->captureSnapshot();
    ASSERT_EQ(next.events.size(), 1u);
    EXPECT_EQ(next.events[0].type, EventType::MousePosition);
    EXPECT_FLOAT_EQ(next.events[0].mouseX, 110.0f);
    EXPECT_FLOAT_EQ(next.events[0].mouseY, 180.0f);
    EXPECT_FLOAT_EQ(next.events[0].mouseDeltaX, 0.0f);
    EXPECT_FLOAT_EQ(next.events[0].mouseDeltaY, 0.0f);
}

TEST_F(InputSnapshotTests, ButtonDragRetainsOriginAndReleaseOutsideTheViewport)
{
    auto *handler = EventHandler::instance();
    handler->setSceneViewportBounds({0.0f, 0.0f}, {100.0f, 100.0f});
    cursor(20.0, 30.0);
    handler->captureSnapshot();
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    cursor(60.0, 70.0);
    const auto input = handler->captureSnapshot();
    const Event *held = findEvent(input, EventType::MouseButtonHeld);
    ASSERT_NE(held, nullptr);
    EXPECT_EQ(held->mouseButton, MouseButton::Left);
    EXPECT_FLOAT_EQ(held->mouseX, 60.0f);
    EXPECT_FLOAT_EQ(held->mouseY, 70.0f);
    EXPECT_FLOAT_EQ(held->dragStartX, 20.0f);
    EXPECT_FLOAT_EQ(held->dragStartY, 30.0f);
    EXPECT_TRUE(held->insideSceneViewport);

    cursor(150.0, 160.0);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
    const auto released = handler->captureSnapshot();
    const Event *release = findEvent(released, EventType::MouseButtonRelease);
    ASSERT_NE(release, nullptr);
    EXPECT_FALSE(release->insideSceneViewport);
    EXPECT_FLOAT_EQ(release->mouseX, 150.0f);
    EXPECT_FLOAT_EQ(release->mouseY, 160.0f);
    EXPECT_FLOAT_EQ(release->dragStartX, 20.0f);
    EXPECT_FLOAT_EQ(release->dragStartY, 30.0f);
    EXPECT_EQ(countEvents(released, EventType::MouseButtonHeld), 0u);
}

TEST_F(InputSnapshotTests, OutsidePressCannotBecomeASceneDragOnEntry)
{
    auto *handler = EventHandler::instance();
    handler->setSceneViewportBounds({0.0f, 0.0f}, {100.0f, 100.0f});
    cursor(150.0, 150.0);
    handler->captureSnapshot();
    mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
    const auto outside = handler->captureSnapshot();
    const Event *press = findEvent(outside, EventType::MouseButtonPress);
    ASSERT_NE(press, nullptr);
    EXPECT_FALSE(press->insideSceneViewport);

    cursor(50.0, 50.0);
    const auto entered = handler->captureSnapshot();
    const Event *held = findEvent(entered, EventType::MouseButtonHeld);
    ASSERT_NE(held, nullptr);
    EXPECT_FALSE(held->insideSceneViewport);
    EXPECT_FLOAT_EQ(held->dragStartX, 150.0f);
    EXPECT_FLOAT_EQ(held->dragStartY, 150.0f);
    EXPECT_EQ(countEvents(entered, EventType::MouseButtonPress), 0u);
    EXPECT_TRUE(entered.events.back().insideSceneViewport);
}

TEST_F(InputSnapshotTests, ScrollAndButtonEventsKeepNativeOrderAndDoNotRepeat)
{
    auto *handler = EventHandler::instance();
    EventHandler::glfwScrollCallback(nullptr, 1.5, -2.0);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    EventHandler::glfwScrollCallback(nullptr, -0.5, 3.0);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
    const auto input = handler->captureSnapshot();
    ASSERT_EQ(input.events.size(), 5u);
    EXPECT_EQ(input.events[0].type, EventType::MouseScroll);
    EXPECT_FLOAT_EQ(input.events[0].scrollX, 1.5f);
    EXPECT_FLOAT_EQ(input.events[0].scrollY, -2.0f);
    EXPECT_EQ(input.events[1].type, EventType::MouseButtonPress);
    EXPECT_EQ(input.events[2].type, EventType::MouseScroll);
    EXPECT_FLOAT_EQ(input.events[2].scrollX, -0.5f);
    EXPECT_FLOAT_EQ(input.events[2].scrollY, 3.0f);
    EXPECT_EQ(input.events[3].type, EventType::MouseButtonRelease);
    const auto drained = handler->captureSnapshot();
    ASSERT_EQ(drained.events.size(), 1u);
    EXPECT_EQ(drained.events[0].type, EventType::MousePosition);
}

TEST_F(InputSnapshotTests, CallbackQueueRetainsAllEventsUntilCapture)
{
    auto *handler = EventHandler::instance();
    const auto previous = handler->captureSnapshot();
    ASSERT_EQ(previous.events.size(), 1u);
    key(GLFW_KEY_A, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_PRESS);
    cursor(10.0, 20.0);
    EventHandler::glfwScrollCallback(nullptr, 0.0, 1.0);
    key(GLFW_KEY_A, GLFW_RELEASE);
    mouse(GLFW_MOUSE_BUTTON_MIDDLE, GLFW_RELEASE);
    const auto input = handler->captureSnapshot();
    ASSERT_EQ(input.events.size(), 7u);
    EXPECT_EQ(input.events[0].type, EventType::KeyPress);
    EXPECT_EQ(input.events[0].keyType, KeyType::A);
    EXPECT_EQ(input.events[1].type, EventType::MouseButtonPress);
    EXPECT_EQ(input.events[1].mouseButton, MouseButton::Middle);
    EXPECT_EQ(input.events[2].type, EventType::MouseMove);
    EXPECT_EQ(input.events[3].type, EventType::MouseScroll);
    EXPECT_EQ(input.events[4].type, EventType::KeyRelease);
    EXPECT_EQ(input.events[4].keyType, KeyType::A);
    EXPECT_EQ(input.events[5].type, EventType::MouseButtonRelease);
    EXPECT_EQ(input.events[5].mouseButton, MouseButton::Middle);
    EXPECT_EQ(countEvents(input, EventType::KeyHeld), 0u);
    EXPECT_EQ(countEvents(input, EventType::MouseButtonHeld), 0u);
    EXPECT_FLOAT_EQ(input.events.back().mouseX, 10.0f);
    EXPECT_FLOAT_EQ(input.events.back().mouseY, 20.0f);

    const auto drained = handler->captureSnapshot();
    ASSERT_EQ(drained.events.size(), 1u);
    EXPECT_EQ(drained.events[0].type, EventType::MousePosition);
}

TEST_F(InputSnapshotTests, CapturedBatchIsImmutableAndBroadcastToEveryReader)
{
    auto *handler = EventHandler::instance();
    handler->setSceneViewportBounds({0.0f, 0.0f}, {100.0f, 100.0f});
    cursor(50.0, 50.0);
    handler->captureSnapshot();
    key(GLFW_KEY_SPACE, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
    const auto captured = handler->captureSnapshot();
    EXPECT_EQ(countEvents(captured, EventType::KeyPress), 1u);
    EXPECT_EQ(countEvents(captured, EventType::KeyPress), 1u);
    const auto frozen = captured;

    key(GLFW_KEY_SPACE, GLFW_RELEASE);
    mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_RELEASE);
    handler->setSceneViewportBounds({200.0f, 200.0f}, {300.0f, 300.0f});
    cursor(60.0, 60.0);
    const auto later = handler->captureSnapshot();
    EXPECT_EQ(countEvents(later, EventType::KeyHeld), 0u);
    EXPECT_EQ(countEvents(later, EventType::MouseButtonHeld), 0u);
    EXPECT_FALSE(later.events.back().insideSceneViewport);
    EXPECT_EQ(captured.events.size(), frozen.events.size());
    EXPECT_EQ(countEvents(captured, EventType::KeyHeld), 1u);
    EXPECT_EQ(countEvents(captured, EventType::MouseButtonHeld), 1u);
    EXPECT_TRUE(captured.events.back().insideSceneViewport);
    EXPECT_FLOAT_EQ(captured.events.back().mouseX, 50.0f);
}

TEST_F(InputSnapshotTests, CapturesEveryEventInOrderBeyondFiveThousand)
{
    key(GLFW_KEY_W, GLFW_PRESS);
    for (std::size_t index = 0; index < 5500; ++index)
    {
        key(GLFW_KEY_W, GLFW_REPEAT);
    }
    key(GLFW_KEY_W, GLFW_RELEASE);
    const auto input = EventHandler::instance()->captureSnapshot();
    ASSERT_EQ(input.events.size(), 5503u);
    EXPECT_EQ(input.events.front().type, EventType::KeyPress);
    for (std::size_t index = 1; index <= 5500; ++index)
    {
        EXPECT_EQ(input.events[index].type, EventType::KeyRepeat);
    }
    EXPECT_EQ(input.events[5501].type, EventType::KeyRelease);
    EXPECT_EQ(input.events.back().type, EventType::MousePosition);
}

TEST_F(InputSnapshotTests, AccumulationPreservesQuickTapsAndReplacesHeldAndPointerSamples)
{
    auto *handler = EventHandler::instance();
    InputSnapshot pending;
    key(GLFW_KEY_SPACE, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    pending.accumulate(handler->captureSnapshot());

    key(GLFW_KEY_SPACE, GLFW_RELEASE);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
    cursor(25.0, 30.0);
    pending.accumulate(handler->captureSnapshot());

    key(GLFW_KEY_D, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
    cursor(50.0, 60.0);
    pending.accumulate(handler->captureSnapshot());

    ASSERT_EQ(pending.events.size(), 11u);
    EXPECT_EQ(pending.events[0].type, EventType::KeyPress);
    EXPECT_EQ(pending.events[0].keyType, KeyType::Space);
    EXPECT_EQ(pending.events[1].type, EventType::MouseButtonPress);
    EXPECT_EQ(pending.events[2].type, EventType::KeyRelease);
    EXPECT_EQ(pending.events[3].type, EventType::MouseButtonRelease);
    EXPECT_EQ(pending.events[4].type, EventType::MouseMove);
    EXPECT_EQ(pending.events[5].type, EventType::KeyPress);
    EXPECT_EQ(pending.events[5].keyType, KeyType::D);
    EXPECT_EQ(pending.events[6].type, EventType::MouseButtonPress);
    EXPECT_EQ(pending.events[7].type, EventType::MouseMove);
    EXPECT_EQ(countEvents(pending, EventType::KeyHeld), 1u);
    EXPECT_EQ(countEvents(pending, EventType::MouseButtonHeld), 1u);
    EXPECT_EQ(countEvents(pending, EventType::MousePosition), 1u);
    const Event *held = findEvent(pending, EventType::KeyHeld);
    ASSERT_NE(held, nullptr);
    EXPECT_EQ(held->keyType, KeyType::D);
    EXPECT_FLOAT_EQ(pending.events.back().mouseX, 50.0f);
    EXPECT_FLOAT_EQ(pending.events.back().mouseY, 60.0f);

    pending.accumulate(handler->captureSnapshot());
    EXPECT_EQ(pending.events.size(), 11u);
    EXPECT_EQ(countEvents(pending, EventType::KeyHeld), 1u);
    EXPECT_EQ(countEvents(pending, EventType::MouseButtonHeld), 1u);
    EXPECT_EQ(countEvents(pending, EventType::MousePosition), 1u);
}

TEST_F(InputSnapshotTests, CatchUpTicksRetainOnlyHeldInputsAndLatestPointer)
{
    auto *handler = EventHandler::instance();
    key(GLFW_KEY_A, GLFW_PRESS);
    key(GLFW_KEY_A, GLFW_RELEASE);
    key(GLFW_KEY_D, GLFW_PRESS);
    key(GLFW_KEY_D, GLFW_REPEAT);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_PRESS);
    mouse(GLFW_MOUSE_BUTTON_LEFT, GLFW_RELEASE);
    mouse(GLFW_MOUSE_BUTTON_RIGHT, GLFW_PRESS);
    cursor(50.0, 60.0);
    EventHandler::glfwScrollCallback(nullptr, 1.0, 2.0);
    auto pending = handler->captureSnapshot();
    const auto firstTick = pending;
    pending.clearTransientState();
    ASSERT_EQ(pending.events.size(), 3u);
    EXPECT_EQ(pending.events[0].type, EventType::KeyHeld);
    EXPECT_EQ(pending.events[0].keyType, KeyType::D);
    EXPECT_EQ(pending.events[1].type, EventType::MouseButtonHeld);
    EXPECT_EQ(pending.events[1].mouseButton, MouseButton::Right);
    EXPECT_EQ(pending.events[2].type, EventType::MousePosition);
    EXPECT_FLOAT_EQ(pending.events[2].mouseX, 50.0f);
    EXPECT_FLOAT_EQ(pending.events[2].mouseY, 60.0f);
    pending.clearTransientState();
    EXPECT_EQ(pending.events.size(), 3u);
    EXPECT_EQ(countEvents(firstTick, EventType::KeyPress), 2u);
    EXPECT_EQ(countEvents(firstTick, EventType::KeyRelease), 1u);
    EXPECT_EQ(countEvents(firstTick, EventType::KeyRepeat), 1u);
    EXPECT_EQ(countEvents(firstTick, EventType::MouseButtonPress), 2u);
    EXPECT_EQ(countEvents(firstTick, EventType::MouseButtonRelease), 1u);
    EXPECT_EQ(countEvents(firstTick, EventType::MouseMove), 1u);
    EXPECT_EQ(countEvents(firstTick, EventType::MouseScroll), 1u);
}

TEST_F(InputSnapshotTests, AccumulatedFramesRetainEveryEventInOrderBeyondTheFormerLimit)
{
    InputSnapshot pending;
    InputSnapshot first;
    first.events.resize(5000, Event{"first", EventType::KeyRepeat, KeyType::F});
    first.events.front() = Event{"press", EventType::KeyPress, KeyType::F};
    pending.accumulate(first);

    InputSnapshot second;
    for (int index = 0; index < 5; ++index)
    {
        second.events.push_back(Event{"second" + std::to_string(index), EventType::KeyRepeat, KeyType::F});
    }
    pending.accumulate(second);
    InputSnapshot release;
    release.events.push_back(Event{"release", EventType::KeyRelease, KeyType::F});
    pending.accumulate(release);
    ASSERT_EQ(pending.events.size(), 5006u);
    EXPECT_EQ(pending.events.front().name, "press");
    EXPECT_EQ(pending.events[4999].name, "first");
    for (std::size_t index = 0; index < second.events.size(); ++index)
    {
        EXPECT_EQ(pending.events[5000 + index].name, second.events[index].name);
    }
    EXPECT_EQ(pending.events.back().name, "release");
    pending.clearTransientState();
    EXPECT_TRUE(pending.events.empty());
    EXPECT_EQ(first.events.size(), 5000u);
}
