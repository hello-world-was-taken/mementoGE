#include "core/Window.h"
#include "core/MouseListener.h"
#include "core/EventHandler.h"
#include "core/Camera.h"

Window::Window(MouseListener listener, EventHandler &eventHandler, float width, float height)
    : m_width(width), m_height(height), m_mouse_listener(listener), mEventHandler(eventHandler)
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

// TODO: Remove all the resources we used
Window::~Window()
{
}

float Window::getWidth() const
{
    return m_width;
}

float Window::getHeight() const
{
    return m_height;
}

void Window::setupWindowHints() const
{
    // Asking for core profile. Should be after glfwInit and before creating a window. Otherwise, it won't work.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    // to avoid Letterboxing since we are using a virtual aspect ratio
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
}

void Window::setupCallBack() const
{
    glfwSetCursorPosCallback(m_glfw_window, m_mouse_listener.cursorPositionCallback);
    glfwSetMouseButtonCallback(m_glfw_window, m_mouse_listener.mouseButtonCallback);
    glfwSetScrollCallback(m_glfw_window, m_mouse_listener.scrollCallback);
    glfwSetKeyCallback(m_glfw_window, mEventHandler.glfwKeyCallBack);
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

/*
    On high DPI displays and Retina Displays, the framebuffer size may be different from the window size. The code below retrieves the size, in pixels, of the framebuffer of the specified window.
    Therefore when we map our frustum to the screen with our field of view, we use the framebuffer size. If you wish to retrieve the size of the window in screen coordinates, see glfwGetWindowSize.

    Initially when the window is created, glViewPort is set to the size of the window. This shouldn't be
    the case, as mentioned above, we should set the viewport to the framebuffer size. This is why we should
    call this function once after the window is created.

    As for when it is resized, Window::frameBufferSizeResizeCallback will update the view port to the new
    framebuffer size.

    https://www.glfw.org/docs/3.3/window_guide.html#window_fbsize

    NOTE: we are now using our own framebuffer to render to with imgui docking. For now the glfwGetFramebufferSize is the same as our custom frame buffer size it is okay.
*/
void Window::updateViewPort()
{
    int vpSize[2];
    glfwGetFramebufferSize(getGlfwWindow(), &vpSize[0], &vpSize[1]);
    glViewport(0, 0, vpSize[0], vpSize[1]);
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
}
