#include "core/MouseListener.h"
#include "core/Window.h"

MouseListener *MouseListener::instance()
{
    static MouseListener instance;
    return &instance;
}

void MouseListener::beginFrame()
{
    // previous mouse position is used to calculate mouse delta and has not been tested yet
    m_prevMousePos = m_mousePos;
    m_scrollDelta = {0.0f, 0.0f};
    m_buttonPressed.clear();
    m_buttonReleased.clear();
}

void MouseListener::endFrame()
{
    // TODO: is there per frame clean up we need to do?
}

void MouseListener::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    auto *listener = MouseListener::instance();

    if (action == GLFW_PRESS)
    {
        // button states is used to track clicks across renders (like dragging)
        // if it is a new button click update dragstart
        if (!listener->m_buttonStates[button])
        {
            listener->m_dragStart = listener->m_mousePos;
        }
        listener->m_buttonStates[button] = true;
        listener->m_buttonPressed[button] = true;
        listener->m_buttonReleased[button] = false;
    }
    else if (action == GLFW_RELEASE)
    {
        listener->m_buttonStates[button] = false;
        listener->m_buttonPressed[button] = false;
        listener->m_buttonReleased[button] = true;
    }
}

void MouseListener::cursorPositionCallback(GLFWwindow *window, double xPos, double yPos)
{
    auto *listener = MouseListener::instance();
    glm::vec2 newMousePos = {static_cast<float>(xPos), static_cast<float>(yPos)};

    listener->m_prevMousePos = listener->m_mousePos;
    listener->m_mousePos = newMousePos;
}

void MouseListener::scrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    auto *listener = MouseListener::instance();
    listener->m_scrollDelta = {static_cast<float>(xOffset), static_cast<float>(yOffset)};
}

glm::vec2 MouseListener::getDragStart() const
{
    return m_dragStart;
}
glm::vec2 MouseListener::getMouseScreenPosition() const
{
    return m_mousePos;
}

glm::vec2 MouseListener::getPrevMouseScreenPosition() const
{
    return m_prevMousePos;
}

// returns the delta in window size coordinate, not game world coordinate
glm::vec2 MouseListener::getMouseDelta() const
{
    auto *listener = MouseListener::instance();

    return {listener->m_mousePos.x - listener->m_prevMousePos.x, listener->m_prevMousePos.y - listener->m_mousePos.y};
}

glm::vec2 MouseListener::getScrollDelta() const
{
    return m_scrollDelta;
}

bool MouseListener::isMouseButtonHeld(int button) const
{
    auto it = m_buttonStates.find(button);
    return it != m_buttonStates.end() && it->second;
}

bool MouseListener::wasMouseButtonPressed(int button) const
{
    // true if this button was pressed at least once this frame.
    // We expect at most one click per frame; multiple clicks in a single frame
    // are extremely unlikely and are treated the same as a single press.
    auto it = m_buttonPressed.find(button);
    return it != m_buttonPressed.end() && it->second;
}

bool MouseListener::wasMouseButtonReleased(int button) const
{
    // true if this button was released at least once this frame.
    // Like presses, we only care that a release occurred during the frame, not
    // how many times.
    auto it = m_buttonReleased.find(button);
    return it != m_buttonReleased.end() && it->second;
}
