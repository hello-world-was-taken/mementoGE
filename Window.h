#include <iostream>
#include "GL/glew.h"
#include <GLFW/glfw3.h>


class Window
{
private:
    static Window *mp_window;
    GLFWwindow* mp_glfw_window = nullptr;
    unsigned int m_width = 1920;
    unsigned int m_height = 1080;
    const char *mp_title = "OpenGL Playground";
    
    void setUpWindowHints() const;
    void initializeWindow();
    void mainLoop() const;
    void setUpCallBack() const;
public:
    Window(/* args */);
    ~Window();

    static Window*  getWindow();
    void run();
};