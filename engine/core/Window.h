#pragma once

#include <iostream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>

#include "util/Time.h"
#include "engine/core/MouseListener.h"
#include "engine/core/KeyListener.h"
#include "engine/core/SceneManager.h"

class SceneManager;
class Window
{
private:
    static Window *m_window;
    std::shared_ptr<SceneManager> m_sceneManager;
    GLFWwindow* m_glfw_window;
    unsigned int m_width = 1920;
    unsigned int m_height = 1080;
    const char *m_title = "OpenGL Playground";

    void setupWindowHints() const;
    void setupImgui() const;
    void showImguiDemo() const;
    void mainLoop();
    void setupCallBack() const;

public:
    Window();
    ~Window();

    static Window *getWindow();

    // addSceneManager should be called before run
    void addSceneManager(std::shared_ptr<SceneManager> sceneManager);
    void initializeWindow();
    GLFWwindow* getGlfwWindow();
    void run();
};