/*  TODO:
    Should implement a dedicated Event System to handle mouse events
    and make it available via mementoGE core api.
*/

#include "core/Window.h"
#include "core/MouseListener.h"

MouseListener *MouseListener::get()
{
    static MouseListener instance;
    return &instance;
}

void MouseListener::beginFrame()
{
    m_mouseDelta = {0.0f, 0.0f};
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
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);

    // TODO: the below one doesn't work with the custom
    // frame buffer. Use a different one.
    // if (ImGui::GetIO().WantCaptureMouse)
    //     return;

    auto *listener = MouseListener::get();

    if (action == GLFW_PRESS)
    {
        listener->m_buttonStates[button] = true;
        listener->m_buttonPressed[button] = true;
    }
    else if (action == GLFW_RELEASE)
    {
        listener->m_buttonStates[button] = false;
        listener->m_buttonReleased[button] = true;
    }
}

void MouseListener::cursorPositionCallback(GLFWwindow *window, double xPos, double yPos)
{
    ImGui_ImplGlfw_CursorPosCallback(window, xPos, yPos);

    // TODO: the below one doesn't work with the custom
    // frame buffer. Use a different one.
    // if (ImGui::GetIO().WantCaptureMouse)
    //     return;

    auto *listener = MouseListener::get();
    glm::vec2 newMousePos = {static_cast<float>(xPos), static_cast<float>(yPos)};

    listener->m_mouseDelta.x = newMousePos.x - listener->m_mousePos.x;
    // Reversed since y-coordinates go from bottom to top
    listener->m_mouseDelta.y = listener->m_mousePos.y - newMousePos.y;

    listener->m_mousePos = newMousePos;
}

void MouseListener::scrollCallback(GLFWwindow *window, double xOffset, double yOffset)
{
    ImGui_ImplGlfw_ScrollCallback(window, xOffset, yOffset);

    // TODO: the below one doesn't work with the custom
    // frame buffer. Use a different one.
    // if (ImGui::GetIO().WantCaptureMouse)
    //     return;

    auto *listener = MouseListener::get();
    listener->m_scrollDelta = {static_cast<float>(xOffset), static_cast<float>(yOffset)};
}

glm::vec2 MouseListener::getMouseScreenPosition() const
{
    return m_mousePos;
}

glm::vec2 MouseListener::getMouseDelta() const
{
    return m_mouseDelta;
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
    return m_buttonPressed.find(button) != m_buttonPressed.end();
}

bool MouseListener::wasMouseButtonReleased(int button) const
{
    return m_buttonReleased.find(button) != m_buttonReleased.end();
}
