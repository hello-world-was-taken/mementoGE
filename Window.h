#include <iostream>
#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include "engine/MouseListener.h"
#include "engine/KeyListener.h"
#include "util/Time.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "game/SceneManager.h"

class Window
{
private:
    static Window *m_window;
    SceneManager *m_scene_manager;
    GLFWwindow *m_glfw_window = nullptr;
    unsigned int m_width = 1920;
    unsigned int m_height = 1080;
    const char *m_title = "OpenGL Playground";

    void setupWindowHints() const;
    void setupImgui() const;
    void showImguiDemo() const;
    void initializeWindow();
    void mainLoop() const;
    void setupCallBack() const;

public:
    Window(/* args */);
    ~Window();

    static Window *getWindow();
    void run();
};