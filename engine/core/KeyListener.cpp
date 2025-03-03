/*  TODO:
    Should implement a dedicated Event System to handle key events
    and make it available via mementoGE core api.
*/

#include "engine/core/KeyListener.h"

KeyListener::KeyListener(){};

KeyListener::~KeyListener(){};

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
            leftKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            leftKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            rightKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            rightKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_UP)
    {
        if (action == GLFW_PRESS)
        {
            upKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            upKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_DOWN)
    {
        if (action == GLFW_PRESS)
        {
            downKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
            downKeyPressed = false;
        }
    }
    else if (key == GLFW_KEY_ESCAPE)
    {
        if (action == GLFW_PRESS)
        {
            escapeKeyPressed = true;
        }
        else if (action == GLFW_RELEASE)
        {
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