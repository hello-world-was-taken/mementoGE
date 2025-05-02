#pragma once

#include <iostream>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <memory>

#include "core/Camera.h"

class MouseListener
{
public:
    static MouseListener *get();

    static void mouseButtonCallback(GLFWwindow *window, int button, int action, int mods);
    static void cursorPositionCallback(GLFWwindow *window, double x_pos, double y_pos);
    static void scrollCallback(GLFWwindow *window, double x_offset, double y_offset);

    void beginFrame();
    void endFrame();

    glm::vec2 getMouseScreenPosition() const;
    glm::vec2 getMouseDelta() const;
    glm::vec2 getScrollDelta() const;

    bool isMouseButtonHeld(int button) const;
    bool wasMouseButtonPressed(int button) const;
    bool wasMouseButtonReleased(int button) const;

private:
    MouseListener() = default;

    glm::vec2 m_mousePos = {0.0f, 0.0f};
    glm::vec2 m_mouseDelta = {0.0f, 0.0f};
    glm::vec2 m_scrollDelta = {0.0f, 0.0f};

    std::unordered_map<int, bool> m_buttonStates;   // current frame state
    std::unordered_map<int, bool> m_buttonPressed;  // pressed this frame
    std::unordered_map<int, bool> m_buttonReleased; // released this frame

};