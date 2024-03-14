#include "engine/core/Window.h"

Window::Window() {}

// TODO: Remove all the resources we used
Window::~Window()
{
}

Window *Window::m_window = nullptr;
float Window::m_width = 1024;
float Window::m_height = 536;

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
    glfwSetMouseButtonCallback(m_glfw_window, mouseButtonCallback);
    glfwSetScrollCallback(m_glfw_window, scrollCallback);
    glfwSetKeyCallback(m_glfw_window, keyCallback);
}

void Window::initializeWindow()
{
    /* Init GLFW */
    if (!glfwInit())
        exit(EXIT_FAILURE);

    setupWindowHints();

    m_glfw_window = glfwCreateWindow(m_width, m_height, m_title, NULL, NULL);
    // TODO: This isn't doing anything right now.
    updateViewPort();
    
    glfwMakeContextCurrent(m_glfw_window);
    glfwSetFramebufferSizeCallback(m_glfw_window, frameBufferSizeResizeCallback);
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

void Window::frameBufferSizeResizeCallback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/*
    On high DPI displays and Retina Displays, the framebuffer size may be different from the window size. The code below retrieves the size, in pixels, of the framebuffer of the specified window.
    Therefore when we map our frustum to the screen with our field of view, we use the framebuffer size. If you wish to retrieve the size of the window in screen coordinates, see glfwGetWindowSize.

    Initially when the window is created, glViewPort is set to the size of the window. This shouldn't be
    the case, as mentioned above, we should set the viewport to the framebuffer size. This is why we should
    call this function once after the window is created.

    As for when it is resized, Window::frameBufferSizeResizeCallback will update the view port to the new
    framebuffer size.

    https://www.glfw.org/docs/3.3/window_guide.html#window_fbsize
*/
void Window::updateViewPort()
{
    int vpSize[2];
    glfwGetFramebufferSize(m_window->getGlfwWindow(), &vpSize[0], &vpSize[1]);
    glViewport(0, 0, vpSize[0], vpSize[1]);
}
