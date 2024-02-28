#include <iostream>
#include "GL/glew.h"
#include <GLFW/glfw3.h>
#include "MouseListener.h"
#include "KeyListener.h"
#include "Draw.h"
#include "util/Time.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"


class Window
{
private:
    static Window *mp_window;
    GLFWwindow* mp_glfw_window = nullptr;
    unsigned int m_width = 500;
    unsigned int m_height = 500;
    const char *mp_title = "OpenGL Playground";
    
    void setupWindowHints() const;
    void setupImgui() const;
    void showImguiDemo() const;
    void initializeWindow();
    void mainLoop() const;
    void setupCallBack() const;
public:
    Window(/* args */);
    ~Window();

    static Window*  getWindow();
    void run();
};