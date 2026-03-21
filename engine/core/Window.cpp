#include "core/Window.h"
#include "core/EventHandler.h"

#include "editor/Constants.h"
#include "editor/EditorCamera.h"

#include "util/Time.h"

// Initialize static member
Window *Window::s_instance = nullptr;

Window::Window()
{
    if (s_instance != nullptr)
    {
        throw std::runtime_error("Window already exists!");
    }
    s_instance = this;

    /* Init GLFW */
    if (!glfwInit())
    {
        exit(EXIT_FAILURE);
    }

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

    // alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
}

// TODO: Remove all the resources we used
Window::~Window()
{
    s_instance = nullptr;
}

Window *Window::instance()
{
    if (s_instance == nullptr)
    {
        throw std::runtime_error("No Window instance exists!");
    }
    return s_instance;
}

float Window::getWindowTime()
{
    return glfwGetTime();
}

std::pair<int, int> Window::getFrameBufferSize() const
{
    int width, height;
    glfwGetFramebufferSize(m_glfw_window, &width, &height);
    return {width, height};
}

std::pair<int, int> Window::getWindowSize() const
{
    int width, height;
    glfwGetWindowSize(m_glfw_window, &width, &height);
    return {width, height};
}

// Calculates the viewport (x, y, width, height) required to fit a specific aspect ratio
// within the current window frame buffer. This handles letterboxing and pillarboxing
// to preserve the target aspect ratio.
Viewport Window::getViewportForAspect(float targetAspect) const
{
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_glfw_window, &fbWidth, &fbHeight);

    float windowAspect = static_cast<float>(fbWidth) / static_cast<float>(fbHeight);

    int vpX = 0, vpY = 0, vpW = fbWidth, vpH = fbHeight;
    if (targetAspect > windowAspect)
    {
        // Camera wider than window: pillarbox (bars top/bottom)
        vpW = fbWidth;
        vpH = static_cast<int>(fbWidth / targetAspect);
        vpY = (fbHeight - vpH) / 2;
    }
    else
    {
        // Camera taller than window: letterbox (bars left/right)
        vpH = fbHeight;
        vpW = static_cast<int>(fbHeight * targetAspect);
        vpX = (fbWidth - vpW) / 2;
    }

    return {vpX, vpY, vpW, vpH};
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
    glfwSetCursorPosCallback(m_glfw_window, EventHandler::glfwCursorPosCallback);
    glfwSetMouseButtonCallback(m_glfw_window, EventHandler::glfwMouseButtonCallback);
    glfwSetScrollCallback(m_glfw_window, EventHandler::glfwScrollCallback);
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

    // for now resizing is only allowed in editor view
    EditorCamera *cam = static_cast<EditorCamera *>(glfwGetWindowUserPointer(window));
    if (cam != nullptr)
    {
        cam->onViewportResize(fbWidth, fbHeight);
    }
}

void Window::closeWindow()
{
    glfwSetWindowShouldClose(m_glfw_window, true);
}

// Set pointers to the camera object, so that we'll be able
// to use it inside glfw callback functions like resizing.
void Window::setUserData(EditorCamera *c)
{
    glfwSetWindowUserPointer(m_glfw_window, c);

    // Trigger initial projection update
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(m_glfw_window, &fbWidth, &fbHeight);
    c->onViewportResize(fbWidth, fbHeight);
}
