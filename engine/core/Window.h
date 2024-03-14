#pragma once

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "util/Time.h"

class Window
{
private:
    static Window *m_window;
    GLFWwindow *m_glfw_window;
    const char *m_title = "OpenGL Playground";

    void setupWindowHints() const;

public:
    static float m_width;
    static float m_height;

public:
    Window();
    ~Window();

    static Window *getWindow();

    void initializeWindow();
    void setupCallBack(
        GLFWcursorposfun cursorPosCallback,
        GLFWmousebuttonfun mouseButtonCallback,
        GLFWscrollfun scrollCallback,
        GLFWkeyfun keyCallback) const;
    GLFWwindow *getGlfwWindow();
    static void frameBufferSizeResizeCallback(GLFWwindow *window, int width, int height);
    static void updateViewPort();
};