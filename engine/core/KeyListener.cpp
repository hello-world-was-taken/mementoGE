/*  TODO:
    Should implement a dedicated Event System to handle key events
    and make it available via mementoGE core api.
*/

#include "core/KeyListener.h"
#include "core/Event.h"

KeyListener::KeyListener() {};

KeyListener::~KeyListener() {};

KeyListener *KeyListener::m_key_listener = nullptr;

bool KeyListener::leftKeyPressed = false;
bool KeyListener::rightKeyPressed = false;
bool KeyListener::upKeyPressed = false;
bool KeyListener::downKeyPressed = false;
bool KeyListener::escapeKeyPressed = false;

KeyListener *KeyListener::getListener()
{
    if (m_key_listener == nullptr)
    {
        m_key_listener = new KeyListener();
    }

    return m_key_listener;
}

void KeyListener::keyCallback(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    // TODO: we need a way to tell which key is pressed. Maybe a bool array.
    if (key == GLFW_KEY_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            Event("LeftKey", EventType::MouseLeftClick, true);
            leftKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            Event("LeftKey", EventType::MouseLeftClick, false);
            leftKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            Event("RightKey", EventType::MouseRightClick, true);
            rightKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            Event("RightKey", EventType::MouseRightClick, false);
            rightKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_UP)
    {
        if (action == GLFW_PRESS)
        {
            Event("UpArrowKey", EventType::Key, true, KeyType::UpArrow);
            upKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            Event("UpArrowKey", EventType::Key, false, KeyType::UpArrow);
            upKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        if (action == GLFW_PRESS)
        {
            Event("DownArrowKey", EventType::Key, false, KeyType::DownArrow);
            downKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            Event("DownArrowKey", EventType::Key, false, KeyType::DownArrow);
            downKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
        if (action == GLFW_PRESS)
        {
            Event("EscapeKey", EventType::Key, true, KeyType::Escape);
            escapeKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            Event("EscapeKey", EventType::Key, false, KeyType::Escape);
            escapeKeyPressed = false;
        }
    }
}

bool KeyListener::isKeyPressed(int key)
{
    if (key == GLFW_KEY_LEFT)
    {
        return leftKeyPressed;
    }
    else if (key == GLFW_KEY_RIGHT)
    {
        return rightKeyPressed;
    }
    else if (key == GLFW_KEY_UP)
    {
        return upKeyPressed;
    }
    else if (key == GLFW_KEY_DOWN)
    {
        return downKeyPressed;
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
        return escapeKeyPressed;
    }
    return false;
}

bool KeyListener::hasActiveEvent()
{
    return mHasActiveEvent;
}