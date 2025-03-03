#pragma once

#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "core/Camera.h"

class MouseListener
{
private:
    static MouseListener *m_mouse_listener;
    bool m_is_left_mouse_clicked;
    bool m_is_dragging;
    float m_lastX;
    float m_lastY;

public:
    MouseListener();
    ~MouseListener();

    static MouseListener *getListener();
    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPositionCallback(GLFWwindow *window, double x_pos, double y_pos);
    static void scrollCallback(GLFWwindow *window, double x_offset, double y_offset);
    glm::vec2 getWorldCoordinates(std::shared_ptr<Camera> camera);
};