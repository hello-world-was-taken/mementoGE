#include "./MouseListener.h"


MouseListener::MouseListener() {}


MouseListener::~MouseListener() {}


MouseListener* MouseListener::mp_mouse_listener = nullptr;


MouseListener* MouseListener::getListener() {
    if (mp_mouse_listener == nullptr) {
        mp_mouse_listener = new MouseListener();
    }

    return mp_mouse_listener;
}


void MouseListener::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    // TODO: what if the user clicks on multiple buttons?
    //       or what if the user is using a mouse with multiple buttons like the gaming ones.
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        std::cout << "Left mouse button pressed" << std::endl;

        MouseListener* listener = getListener();
        listener->m_is_left_mouse_clicked = true;
    } else if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE) {
        std::cout << "Left mouse button released" << std::endl;

        MouseListener* listener = getListener();
        listener->m_is_left_mouse_clicked = false;
        listener->m_is_dragging = false;
    }
}


void MouseListener::cursorPositionCallback(GLFWwindow* window, double x_pos, double y_pos) {
    MouseListener* listener = getListener();
    if (listener->m_is_left_mouse_clicked) {
        double x_offset = x_pos - listener->m_lastX;
        double y_offset = listener->m_lastY - y_pos; // Reversed since y-coordinates go from bottom to top

        std::cout << "Mouse dragged: xOffset = " << x_offset << ", yOffset = " << y_offset << std::endl;

        listener->m_lastX = x_pos;
        listener->m_lastY = y_pos;
        listener->m_is_dragging = true;
    } else {
        // Update last cursor position even when not dragging
        listener->m_lastX = x_pos;
        listener->m_lastY = y_pos;
        listener->m_is_dragging = false;
    }
}

void MouseListener::scrollCallback(GLFWwindow* window, double x_offset, double y_offset) {
    std::cout << "Scrolled: xoffset = " << x_offset << ", yoffset = " << y_offset << std::endl;
}