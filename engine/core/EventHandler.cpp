#include <iostream>

#include "core/EventHandler.h"

EventHandler::EventHandler()
{
    std::cout << "Event Handler constructor" << std::endl;
}
EventHandler::~EventHandler()
{
    std::cout << "Event Handler destructor" << std::endl;
}

bool EventHandler::mHasActiveEvent = false;
Event EventHandler::mCurrentEvent = Event("", EventType::None, false);

void EventHandler::glfwKeyCallBack(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // TODO: can we handle this in a better way?
    if (action == GLFW_PRESS)
    {
        mHasActiveEvent = true;
    }
    else
    {
        mHasActiveEvent = false;
    }

    if (key == GLFW_KEY_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            mCurrentEvent = Event("LeftArrowKey", EventType::Key, true, KeyType::LeftArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            mCurrentEvent = Event("LeftArrowKey", EventType::MouseLeftClick, false, KeyType::LeftArrow);
        }
    }
    else if (key == GLFW_KEY_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            mCurrentEvent = Event("RightArrowKey", EventType::Key, true, KeyType::RightArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            mCurrentEvent = Event("RightArrowKey", EventType::Key, false, KeyType::RightArrow);
        }
    }
    else if (key == GLFW_KEY_UP)
    {
        if (action == GLFW_PRESS)
        {
            mCurrentEvent = Event("UpArrowKey", EventType::Key, true, KeyType::UpArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            mCurrentEvent = Event("UpArrowKey", EventType::Key, false, KeyType::UpArrow);
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        if (action == GLFW_PRESS)
        {
            mCurrentEvent = Event("DownArrowKey", EventType::Key, false, KeyType::DownArrow);
        }
        else if (action == GLFW_RELEASE)
        {
            mCurrentEvent = Event("DownArrowKey", EventType::Key, false, KeyType::DownArrow);
        }
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
        if (action == GLFW_PRESS)
        {
            mCurrentEvent = Event("EscapeKey", EventType::Key, true, KeyType::Escape);
        }
        else if (action == GLFW_RELEASE)
        {
            mCurrentEvent = Event("EscapeKey", EventType::Key, false, KeyType::Escape);
        }
    }
}

bool EventHandler::hasActiveEvent()
{
    return mHasActiveEvent;
}

Event EventHandler::getCurrentEvent()
{
    return mCurrentEvent;
}