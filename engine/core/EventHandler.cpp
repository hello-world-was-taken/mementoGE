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
            bool isPressed = (action != GLFW_RELEASE);
            const EventType eventType = (action == GLFW_REPEAT) ? EventType::KeyRepeat : EventType::Key;
            listener->m_keyStates[type] = isPressed;
            listener->m_eventQueue.push(Event{name, eventType, isPressed, type, cmd, ctrl, shift, alt});
        }
    };

    // Movement keys
    updateKey(GLFW_KEY_LEFT, KeyType::LeftArrow, "LeftArrowKey");
    updateKey(GLFW_KEY_RIGHT, KeyType::RightArrow, "RightArrowKey");
    updateKey(GLFW_KEY_UP, KeyType::UpArrow, "UpArrowKey");
    updateKey(GLFW_KEY_DOWN, KeyType::DownArrow, "DownArrowKey");
    updateKey(GLFW_KEY_ESCAPE, KeyType::Escape, "EscapeKey");

    // WASD and Space
    updateKey(GLFW_KEY_W, KeyType::W, "WKey");
    updateKey(GLFW_KEY_A, KeyType::A, "AKey");
    updateKey(GLFW_KEY_S, KeyType::S, "SKey");
    updateKey(GLFW_KEY_D, KeyType::D, "DKey");
    updateKey(GLFW_KEY_SPACE, KeyType::Space, "SpaceKey");

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