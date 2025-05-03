#include <iostream>
#include <imgui.h>

#include "core/EventHandler.h"

EventHandler *EventHandler::get()
{
    static EventHandler instance;
    return &instance;
}

void EventHandler::glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    auto *listener = EventHandler::get();
    ImGuiIO &io = ImGui::GetIO();
    if (io.WantCaptureKeyboard)
    {
        return;
    }
    // TODO: can we handle this in a better way?
    if (action == GLFW_PRESS)
    {
        listener->m_hasActiveEvent = true;
    }
    else
    {
        listener->m_hasActiveEvent = false;
    }

    if (key == GLFW_KEY_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            listener->m_currentEvent = Event("LeftArrowKey", EventType::Key, true, KeyType::LeftArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            listener->m_currentEvent = Event("LeftArrowKey", EventType::MouseLeftClick, false, KeyType::LeftArrow);
        }
    }
    else if (key == GLFW_KEY_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            listener->m_currentEvent = Event("RightArrowKey", EventType::Key, true, KeyType::RightArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            listener->m_currentEvent = Event("RightArrowKey", EventType::Key, false, KeyType::RightArrow);
        }
    }
    else if (key == GLFW_KEY_UP)
    {
        if (action == GLFW_PRESS)
        {
            listener->m_currentEvent = Event("UpArrowKey", EventType::Key, true, KeyType::UpArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            listener->m_currentEvent = Event("UpArrowKey", EventType::Key, false, KeyType::UpArrow);
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        if (action == GLFW_PRESS)
        {
            listener->m_currentEvent = Event("DownArrowKey", EventType::Key, false, KeyType::DownArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            listener->m_currentEvent = Event("DownArrowKey", EventType::Key, false, KeyType::DownArrow);
        }
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
        if (action == GLFW_PRESS)
        {
            listener->m_currentEvent = Event("EscapeKey", EventType::Key, true, KeyType::Escape);
        }
        else if (action == GLFW_RELEASE)
        {
            listener->m_currentEvent = Event("EscapeKey", EventType::Key, false, KeyType::Escape);
        }
    }
}

bool EventHandler::hasActiveEvent()
{
    return m_hasActiveEvent;
}

Event EventHandler::getCurrentEvent()
{
    return m_currentEvent;
}