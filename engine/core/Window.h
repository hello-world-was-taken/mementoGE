#pragma once

#include "core/GLIncludes.h"
#include "util/Time.h"
#include "core/EditorCamera.h"

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
    Window();
    ~Window();

    static void frameBufferSizeResizeCallback(GLFWwindow *window, int width, int height);
    static float getWindowTime();

    void initializeWindow();
    void run(const std::function<void()> &frameFunc, const std::function<void()> &cleanupFunc);

    void setupCallBack() const;
    void setUserData(EditorCamera *c);
    GLFWwindow *getGlfwWindow();

    float getWidth() const;
    float getHeight() const;

    void closeWindow();

private:
    GLFWwindow *m_glfw_window;
    void setupWindowHints() const;
};