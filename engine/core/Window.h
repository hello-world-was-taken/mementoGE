#pragma once

#include "core/GLIncludes.h"
#include "util/Time.h"
#include "core/Camera.h"

#include <iostream>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

// TODO: Window should not have an api that exposes internal
// libraries it uses like GLFW. The user should be able to
// implement their own version of Window and use it without
// changine outside interaction.
class Window
{
public:
    Window(float m_width, float m_height);
    ~Window();

    static void frameBufferSizeResizeCallback(GLFWwindow *window, int width, int height);
    static float getWindowTime();

    void initializeWindow();
    void run(const std::function<void()> &frameFunc, const std::function<void()> &cleanupFunc);

    void setupCallBack() const;
    void setUserData(Camera *c);
    GLFWwindow *getGlfwWindow();

    float getWidth() const;
    float getHeight() const;

    void closeWindow();

public:
    float m_width;
    float m_height;

private:
    GLFWwindow *m_glfw_window;
    const char *m_title = "MementoGE";
    void setupWindowHints() const;
};