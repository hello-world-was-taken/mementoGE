/*  TODO:
    Should implement a dedicated Event System to handle mouse events
    and make it available via mementoGE core api.
*/

#include "game/MouseListener.h"

MouseListener::MouseListener() {}

MouseListener::~MouseListener() {}

MouseListener *MouseListener::m_mouse_listener = nullptr;

MouseListener *MouseListener::getListener()
{
    if (m_mouse_listener == nullptr)
    {
        m_mouse_listener = new MouseListener();
    }

    return m_mouse_listener;
}

void MouseListener::mouseButtonCallback(GLFWwindow *window, int button, int action, int mods)
{
    // TODO: what if the user clicks on multiple buttons?
    //       or what if the user is using a mouse with multiple buttons like the gaming ones.
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {

        MouseListener *listener = getListener();
        listener->m_is_left_mouse_clicked = true;
    }
    else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {

        MouseListener *listener = getListener();
        listener->m_is_left_mouse_clicked = false;
        listener->m_is_dragging = false;
    }
}

void MouseListener::cursorPositionCallback(GLFWwindow *window, double x_pos, double y_pos)
{
    MouseListener *listener = getListener();
    if (listener->m_is_left_mouse_clicked)
    {
        double x_offset = x_pos - listener->m_lastX;
        double y_offset = listener->m_lastY - y_pos; // Reversed since y-coordinates go from bottom to top

        std::cout << "X position: " << x_pos << " "
                  << "Y position: " << y_pos << std::endl;
        listener->m_lastX = x_pos;
        listener->m_lastY = y_pos;
        listener->m_is_dragging = true;
    }
    else
    {
        // Update last cursor position even when not dragging
        listener->m_lastX = x_pos;
        listener->m_lastY = y_pos;
        listener->m_is_dragging = false;
    }
}

void MouseListener::scrollCallback(GLFWwindow *window, double x_offset, double y_offset)
{
}