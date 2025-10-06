#include "core/Window.h"
#include "core/MouseListener.h"
#include "core/EventHandler.h"
#include "core/Camera.h"
#include "core/Constants.h"

#include "util/Time.h"

Window::Window()
{
    /* Init GLFW */
    if (!glfwInit())
        exit(EXIT_FAILURE);

    setupWindowHints();

    m_glfw_window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_TITLE, NULL, NULL);

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
    glEnable(GL_DEPTH_TEST);
}

// TODO: Remove all the resources we used
Window::~Window()
{
}

float Window::getWindowTime()
{
    return glfwGetTime();
}

void Window::run(const std::function<void()> &frameFunc, const std::function<void()> &cleanupFunc)
{
    glfwSwapInterval(1);

    while (!glfwWindowShouldClose(getGlfwWindow()))
    {
        Time::update();
        glfwPollEvents();
        frameFunc();
        glfwSwapBuffers(getGlfwWindow());
    }

    cleanupFunc();
    glfwTerminate();
}

float Window::getWidth() const
{
    return WINDOW_WIDTH;
}

float Window::getHeight() const
{
    return WINDOW_HEIGHT;
}

void Window::setupWindowHints() const
{
    // Asking for core profile. Should be after glfwInit and before creating a window. Otherwise, it won't work.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void Window::setupCallBack() const
{
    glfwSetCursorPosCallback(m_glfw_window, MouseListener::cursorPositionCallback);
    glfwSetMouseButtonCallback(m_glfw_window, MouseListener::mouseButtonCallback);
    glfwSetScrollCallback(m_glfw_window, MouseListener::scrollCallback);
    glfwSetKeyCallback(m_glfw_window, EventHandler::glfwKeyCallBack);
}

void Window::initializeWindow()
{
}

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
    // using framebuffer size instead of window size
    // because of the difference in window size and actual
    // pixels for high DPI devices like mac retina dispaly.
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);

    Camera *cam = static_cast<Camera *>(glfwGetWindowUserPointer(window));
    if (cam != nullptr)
    {
        cam->onWindowResize(fbWidth, fbHeight);
    }
}

void Window::closeWindow()
{
    glfwSetWindowShouldClose(m_glfw_window, true);
}

// Set pointers to the camera object, so that we'll be able
// to use it inside glfw callback functions like resizing.
void Window::setUserData(Camera *c)
{
    glfwSetWindowUserPointer(m_glfw_window, c);

    // Trigger initial projection update
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_glfw_window, &fbWidth, &fbHeight);
    c->onWindowResize(fbWidth, fbHeight);
}
