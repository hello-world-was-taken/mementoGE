#pragma once

#include "core/GLIncludes.h"

#include "editor/EditorCamera.h"

#include "util/Time.h"

#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <imgui.h>
#include <iostream>

struct Viewport
{
    int x, y, width, height;
};

// TODO: Window should not have an api that exposes internal
// libraries it uses like GLFW. The user should be able to
// implement their own version of Window and use it without
// changine outside interaction.
class Window
{
public:
    Window();
    ~Window();

    static Window *instance();

    static void frameBufferSizeResizeCallback(GLFWwindow *window, int width, int height);
    static float getWindowTime();

    void initializeWindow();

    void setupCallBack() const;
    void setUserData(EditorCamera *c);
    GLFWwindow *getGlfwWindow();

    std::pair<int, int> getFrameBufferSize() const;
    std::pair<int, int> getWindowSize() const;

    Viewport getViewportForAspect(float targetAspect) const;

    void closeWindow();

private:
    static Window *s_instance;
    GLFWwindow *m_glfw_window;
    void setupWindowHints() const;
};