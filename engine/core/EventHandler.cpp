#include "core/EventHandler.h"

#include <iostream>
#include <imgui.h>

EventHandler *EventHandler::get()
{
    static EventHandler instance;
    return &instance;
}

void EventHandler::glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto *listener = EventHandler::get();
    // ImGuiIO &io = ImGui::GetIO();
    // if (io.WantCaptureKeyboard)
    // {
    //     return;
    // }

    listener->m_hasActiveEvent = (action == GLFW_PRESS || action == GLFW_REPEAT);

    auto updateKey = [&](int glfwKey, KeyType type, const std::string &name)
    {
        if (key == glfwKey)
        {
            bool isPressed = (action != GLFW_RELEASE);
            const EventType eventType = (action == GLFW_REPEAT) ? EventType::KeyRepeat : EventType::Key;
            listener->m_keyStates[type] = true;
            listener->m_currentEvent = Event{name, eventType, isPressed, type};
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
}

bool EventHandler::hasActiveEvent()
{
    return m_hasActiveEvent;
}

const Event EventHandler::getCurrentEvent()
{
    return m_currentEvent;
}

bool EventHandler::isKeyPressed(KeyType key) const
{
    auto it = m_keyStates.find(key);
    return it != m_keyStates.end() && it->second;
}