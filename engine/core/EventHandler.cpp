#include "core/EventHandler.h"

#include <imgui.h>
#include <iostream>

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
                listener->m_eventQueue.push(Event{name, EventType::Key, true, type, cmd, ctrl, shift, alt});
                break;
            case GLFW_REPEAT:
                listener->m_keyStates[type] = true;
                listener->m_eventQueue.push(Event{name, EventType::KeyRepeat, true, type, cmd, ctrl, shift, alt});
                break;
            case GLFW_RELEASE:
                listener->m_keyStates[type] = false;
                listener->m_eventQueue.push(Event{name, EventType::KeyRelease, false, type, cmd, ctrl, shift, alt});
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

    // WASD, Space, Enter
    updateKey(GLFW_KEY_W, KeyType::W, "WKey");
    updateKey(GLFW_KEY_A, KeyType::A, "AKey");
    updateKey(GLFW_KEY_S, KeyType::S, "SKey");
    updateKey(GLFW_KEY_D, KeyType::D, "DKey");
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

bool EventHandler::hasPendingEvents() const
{
    return !m_eventQueue.empty();
}

Event EventHandler::nextEvent()
{
    if (m_eventQueue.empty())
    {
        return Event{"None", EventType::None, false, KeyType::None};
    }

    Event event = m_eventQueue.front();
    m_eventQueue.pop();

    return event;
}

bool EventHandler::isKeyPressed(KeyType key) const
{
    auto it = m_keyStates.find(key);
    return it != m_keyStates.end() && it->second;
}

void EventHandler::beginFrame()
{
    m_prevMousePos = m_mousePos;
    m_scrollDelta = {0.0f, 0.0f};
    m_mouseButtonPressedFrame.clear();
    m_mouseButtonReleasedFrame.clear();
}

void EventHandler::glfwMouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    auto *listener = EventHandler::instance();

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
        listener->m_mouseButtonReleasedFrame[button] = false;

        event.name = "MouseButtonPress_" + std::to_string(button);
        event.type = EventType::MouseButtonPress;
        listener->m_eventQueue.push(event);
    }
    else if (action == GLFW_RELEASE)
    {
        listener->m_mouseButtonStates[button] = false;
        listener->m_mouseButtonPressedFrame[button] = false;
        listener->m_mouseButtonReleasedFrame[button] = true;

        event.name = "MouseButtonRelease_" + std::to_string(button);
        event.type = EventType::MouseButtonRelease;
        listener->m_eventQueue.push(event);
    }
}

void EventHandler::glfwCursorPosCallback(GLFWwindow *window, double xPos, double yPos)
{
    auto *listener = EventHandler::instance();
    listener->m_prevMousePos = listener->m_mousePos;
    listener->m_mousePos = {static_cast<float>(xPos), static_cast<float>(yPos)};

    // TODO: I don't think we need push an event for cursor position.
    // Storing the location should be fine.
}

void EventHandler::glfwScrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    auto *listener = EventHandler::instance();
    listener->m_scrollDelta = {static_cast<float>(xOffset), static_cast<float>(yOffset)};

    Event event;
    event.name = "MouseScroll";
    event.type = EventType::MouseScroll;
    event.scrollX = static_cast<float>(xOffset);
    event.scrollY = static_cast<float>(yOffset);
    listener->m_eventQueue.push(event);
}

bool EventHandler::isMouseButtonHeld(int button) const
{
    auto it = m_mouseButtonStates.find(button);
    return it != m_mouseButtonStates.end() && it->second;
}

bool EventHandler::wasMouseButtonPressed(int button) const
{
    // true if this button was pressed at least once this frame.
    // We expect at most one click per frame; multiple clicks in a single frame
    // are extremely unlikely and are treated the same as a single press.
    auto it = m_mouseButtonPressedFrame.find(button);
    return it != m_mouseButtonPressedFrame.end() && it->second;
}

bool EventHandler::wasMouseButtonReleased(int button) const
{
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
    return {m_mousePos.x - m_prevMousePos.x, m_prevMousePos.y - m_mousePos.y};
}

glm::vec2 EventHandler::getScrollDelta() const
{
    return m_scrollDelta;
}

glm::vec2 EventHandler::getDragStart() const
{
    return m_dragStart;
}