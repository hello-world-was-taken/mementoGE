#include "engine/core/Window.h"

Window::Window() {}

// TODO: Remove all the resources we used
Window::~Window()
{
}

Window *Window::m_window = nullptr;

void Window::setupWindowHints() const
{
    /* Asking for core profile. Should be after glfwInit and before creating a windo. Otherwise, it won't work.*/
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    std::cout << "Running on Mac" << std::endl;
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void Window::setupCallBack(
    GLFWcursorposfun cursorPositionCallback,
    GLFWmousebuttonfun mouseButtonCallback,
    GLFWscrollfun scrollCallback,
    GLFWkeyfun keyCallback) const
{
    glfwSetCursorPosCallback(m_glfw_window, cursorPositionCallback);
    glfwSetMouseButtonCallback(m_glfw_window,mouseButtonCallback);
    glfwSetScrollCallback(m_glfw_window, scrollCallback);
    glfwSetKeyCallback(m_glfw_window,keyCallback);
}

void Window::initializeWindow()
{
    /* Init GLFW */
    if (!glfwInit())
        exit(EXIT_FAILURE);

    setupWindowHints();

    m_glfw_window = glfwCreateWindow(m_width, m_height, m_title, NULL, NULL);

    glfwMakeContextCurrent(m_glfw_window);
    glfwSwapInterval(1);
    if (!m_glfw_window)
    {
        glfwTerminate();
        exit(EXIT_FAILURE);
    }

    /* Initialize Glew. Must be done after glfw is initialized!*/
    GLenum res = glewInit();
    if (res != GLEW_OK)
    {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return;
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}

Window *Window::getWindow()
{
    if (m_window == nullptr)
    {
        m_window = new Window();
    }

    return m_window;
};

GLFWwindow *Window::getGlfwWindow()
{
    if (m_glfw_window == nullptr)
    {
        std::cout << "GLFW window is null" << std::endl;
        return nullptr;
    }
    return m_glfw_window;
}
