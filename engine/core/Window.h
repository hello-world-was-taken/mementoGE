#pragma once

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "util/Time.h"
#include "core/MouseListener.h"
#include "core/EventHandler.h"

// TODO: Window should not have an api that exposes internal
// libraries it uses like GLFW. The user should be able to
// implement their own version of Window and use it without
// changine outside interaction.
class Window
{
public:
    // TODO: default width height needs to go
    Window(EventHandler &eventHandler, float m_width = 1280, float m_height = 720);
    ~Window();

    void initializeWindow();
    void setupCallBack() const;
    GLFWwindow *getGlfwWindow();
    static void frameBufferSizeResizeCallback(GLFWwindow *window, int width, int height);
    void updateViewPort();
    float getWidth() const;
    float getHeight() const;
    void closeWindow();
    void setUserData(Camera *c);

public:
    float m_width;
    float m_height;
    EventHandler mEventHandler;

private:
    GLFWwindow *m_glfw_window;
    const char *m_title = "MementoGE";
    void setupWindowHints() const;
};