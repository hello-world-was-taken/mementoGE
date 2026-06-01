#include "core/EventHandler.h"

#include <utility>

EventHandler *EventHandler::instance()
{
    static EventHandler instance;
    return &instance;
}

void EventHandler::glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto *listener = EventHandler::instance();

    // modifiers
    bool cmd = mods & GLFW_MOD_SUPER;
    bool ctrl = mods & GLFW_MOD_CONTROL;
    bool shift = mods & GLFW_MOD_SHIFT;
    bool alt = mods & GLFW_MOD_ALT;

    auto updateKey = [&](int glfwKey, KeyType type, const std::string &name)
    {
        if (key == glfwKey)
        {
            switch (action)
            {
            case GLFW_PRESS:
                listener->m_keyStates[type] = true;
                listener->m_keyPressedFrame[type] = true;
                listener->recordEvent(Event{name, EventType::Key, true, type, cmd, ctrl, shift, alt});
                break;
            case GLFW_REPEAT:
                listener->m_keyStates[type] = true;
                listener->recordEvent(Event{name, EventType::KeyRepeat, true, type, cmd, ctrl, shift, alt});
                break;
            case GLFW_RELEASE:
                listener->m_keyStates[type] = false;
                listener->m_keyReleasedFrame[type] = true;
                listener->recordEvent(Event{name, EventType::KeyRelease, false, type, cmd, ctrl, shift, alt});
                break;
            }
        }
    };

    // Movement keys
    updateKey(GLFW_KEY_LEFT, KeyType::LeftArrow, "LeftArrowKey");
    updateKey(GLFW_KEY_RIGHT, KeyType::RightArrow, "RightArrowKey");
    updateKey(GLFW_KEY_UP, KeyType::UpArrow, "UpArrowKey");
    updateKey(GLFW_KEY_DOWN, KeyType::DownArrow, "DownArrowKey");
    updateKey(GLFW_KEY_ESCAPE, KeyType::Escape, "EscapeKey");
    updateKey(GLFW_KEY_LEFT_SHIFT, KeyType::LeftShift, "LeftShiftKey");
    updateKey(GLFW_KEY_RIGHT_SHIFT, KeyType::RightShift, "RightShiftKey");

    // WASD, Space, Enter
    updateKey(GLFW_KEY_W, KeyType::W, "WKey");
    updateKey(GLFW_KEY_A, KeyType::A, "AKey");
    updateKey(GLFW_KEY_S, KeyType::S, "SKey");
    updateKey(GLFW_KEY_D, KeyType::D, "DKey");
    updateKey(GLFW_KEY_F, KeyType::F, "FKey");
    updateKey(GLFW_KEY_SPACE, KeyType::Space, "SpaceKey");

    updateKey(GLFW_KEY_ENTER, KeyType::Enter, "EnterKey");

    updateKey(GLFW_KEY_M, KeyType::M, "MKey");

    // Copy / Paste
    updateKey(GLFW_KEY_C, KeyType::C, "CKey");
    updateKey(GLFW_KEY_V, KeyType::V, "VKey");

    // Z, Y Undo / Redo
    updateKey(GLFW_KEY_Y, KeyType::Y, "YKey");
    updateKey(GLFW_KEY_Z, KeyType::Z, "ZKey");
}

void EventHandler::recordEvent(Event event)
{
    m_events.push_back(std::move(event));
}

InputSnapshot EventHandler::captureSnapshot()
{
    InputSnapshot snapshot;
    snapshot.keyHeld = m_keyStates;
    snapshot.keyPressed = m_keyPressedFrame;
    snapshot.keyReleased = m_keyReleasedFrame;
    snapshot.mouseHeld = m_mouseButtonStates;
    snapshot.mousePressed = m_mouseButtonPressedFrame;
    snapshot.mouseReleased = m_mouseButtonReleasedFrame;
    snapshot.mousePos = m_mousePos;
    snapshot.mouseDelta = m_mouseDelta;
    snapshot.scrollDelta = m_scrollDelta;
    snapshot.dragStart = m_dragStart;
    snapshot.sceneViewportMin = m_sceneViewportMin;
    snapshot.sceneViewportMax = m_sceneViewportMax;
    snapshot.hasSceneViewportBounds = m_hasSceneViewportBounds;
    snapshot.events = std::move(m_events);
    m_events.clear();
    return snapshot;
}

bool EventHandler::isKeyPressed(KeyType key) const
{
    auto it = m_keyStates.find(key);
    return it != m_keyStates.end() && it->second;
}

void EventHandler::beginFrame()
{
    m_mouseDelta = {0.0f, 0.0f};
    m_scrollDelta = {0.0f, 0.0f};
    m_keyPressedFrame.clear();
    m_keyReleasedFrame.clear();
    m_mouseButtonPressedFrame.clear();
    m_mouseButtonReleasedFrame.clear();
    m_events.clear();
}

void EventHandler::glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    auto *listener = EventHandler::instance();

    const bool mouseInsideSceneViewport = listener->isMouseInsideSceneViewport();
    const bool buttonWasPressedInScene = listener->m_mouseButtonStates[button];
    if ((action == GLFW_PRESS && !mouseInsideSceneViewport) || (action == GLFW_RELEASE && !buttonWasPressedInScene))
    {
        return;
    }

    // modifiers
    bool cmd = mods & GLFW_MOD_SUPER;
    bool ctrl = mods & GLFW_MOD_CONTROL;
    bool shift = mods & GLFW_MOD_SHIFT;
    bool alt = mods & GLFW_MOD_ALT;

    Event event;
    event.isPressed = (action == GLFW_PRESS);
    event.mouseButton = button;
    event.mouseX = listener->m_mousePos.x;
    event.mouseY = listener->m_mousePos.y;
    event.cmd = cmd;
    event.ctrl = ctrl;
    event.shift = shift;
    event.alt = alt;

    if (action == GLFW_PRESS)
    {
        if (!listener->m_mouseButtonStates[button])
        {
            listener->m_dragStart = listener->m_mousePos;
        }

        listener->m_mouseButtonStates[button] = true;
        listener->m_mouseButtonPressedFrame[button] = true;

        event.name = "MouseButtonPress_" + std::to_string(button);
        event.type = EventType::MouseButtonPress;
        listener->recordEvent(std::move(event));
    }
    else if (action == GLFW_RELEASE)
    {
        listener->m_mouseButtonStates[button] = false;
        listener->m_mouseButtonReleasedFrame[button] = true;

        event.name = "MouseButtonRelease_" + std::to_string(button);
        event.type = EventType::MouseButtonRelease;
        listener->recordEvent(std::move(event));
    }
}

void EventHandler::glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos)
{
    auto *listener = EventHandler::instance();
    const glm::vec2 nextPosition{static_cast<float>(xPos), static_cast<float>(yPos)};
    listener->m_mouseDelta +=
        glm::vec2{nextPosition.x - listener->m_mousePos.x, listener->m_mousePos.y - nextPosition.y};
    listener->m_mousePos = nextPosition;

    // TODO: I don't think we need push an event for cursor position.
    // Storing the location should be fine.
}

void EventHandler::glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    auto *listener = EventHandler::instance();
    listener->m_scrollDelta += glm::vec2{static_cast<float>(xOffset), static_cast<float>(yOffset)};

    Event event;
    event.name = "MouseScroll";
    event.type = EventType::MouseScroll;
    event.scrollX = static_cast<float>(xOffset);
    event.scrollY = static_cast<float>(yOffset);
    listener->recordEvent(std::move(event));
}

bool EventHandler::isMouseButtonHeld(int button) const
{
    if (!isMouseInsideSceneViewport())
    {
        return false;
    }

    auto it = m_mouseButtonStates.find(button);
    return it != m_mouseButtonStates.end() && it->second;
}

bool EventHandler::wasMouseButtonPressed(int button) const
{
    if (!isMouseInsideSceneViewport())
    {
        return false;
    }

    // true if this button was pressed at least once this frame.
    // We expect at most one click per frame; multiple clicks in a single frame
    // are extremely unlikely and are treated the same as a single press.
    auto it = m_mouseButtonPressedFrame.find(button);
    return it != m_mouseButtonPressedFrame.end() && it->second;
}

bool EventHandler::wasMouseButtonReleased(int button) const
{
    if (!isMouseInsideSceneViewport())
    {
        return false;
    }

    // true if this button was released at least once this frame.
    // Like presses, we only care that a release occurred during the frame, not
    // how many times.
    auto it = m_mouseButtonReleasedFrame.find(button);
    return it != m_mouseButtonReleasedFrame.end() && it->second;
}

glm::vec2 EventHandler::getMousePos() const
{
    return m_mousePos;
}

// returns the delta in window size coordinate, not game world coordinate
glm::vec2 EventHandler::getMouseDelta() const
{
    if (!isMouseInsideSceneViewport())
    {
        return {0.0f, 0.0f};
    }

    return m_mouseDelta;
}

glm::vec2 EventHandler::getScrollDelta() const
{
    return m_scrollDelta;
}

glm::vec2 EventHandler::getDragStart() const
{
    return m_dragStart;
}

void EventHandler::setSceneViewportBounds(glm::vec2 min, glm::vec2 max)
{
    m_sceneViewportMin = min;
    m_sceneViewportMax = max;
    m_hasSceneViewportBounds = true;
}

bool EventHandler::isMouseInsideSceneViewport() const
{
    if (!m_hasSceneViewportBounds)
    {
        return true;
    }

    return m_mousePos.x >= m_sceneViewportMin.x && m_mousePos.x <= m_sceneViewportMax.x &&
           m_mousePos.y >= m_sceneViewportMin.y && m_mousePos.y <= m_sceneViewportMax.y;
}
