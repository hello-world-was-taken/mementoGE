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