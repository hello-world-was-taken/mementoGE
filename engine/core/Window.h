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
public:
    Window(float m_width = 800, float m_height = 600);
    ~Window();

    void initializeWindow();
    void setupCallBack(GLFWcursorposfun cursorPosCallback, GLFWmousebuttonfun mouseButtonCallback, GLFWscrollfun scrollCallback, GLFWkeyfun keyCallback) const;
    GLFWwindow *getGlfwWindow();
    static void frameBufferSizeResizeCallback(GLFWwindow *window, int width, int height);
    void updateViewPort();
    float getWidth() const;
    float getHeight() const;

public:
    float m_width;
    float m_height;

private:
    GLFWwindow *m_glfw_window;
    const char *m_title = "OpenGL Playground";

    void setupWindowHints() const;
};