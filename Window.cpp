#include "./Window.h"
#include "./MouseListener.h"
#include "./KeyListener.h"
#include "./opengl/draw.h"
#include "./util/Time.h"


Window::Window(/* args */) {}


// TODO: Remove all the resources we used
Window::~Window() {}


Window* Window::mp_window = nullptr;


void Window::setUpWindowHints() const {
    /* Asking for core profile. Should be after glfwInit and before creating a windo. Otherwise, it won't work.*/
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    std::cout << "Running on Mac" << std::endl;
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif
}


void Window::setUpCallBack() const {
    glfwSetCursorPosCallback(mp_glfw_window, MouseListener::cursorPositionCallback);
    glfwSetMouseButtonCallback(mp_glfw_window, MouseListener::mouseButtonCallback);
    glfwSetScrollCallback(mp_glfw_window, MouseListener::scrollCallback);
    glfwSetKeyCallback(mp_glfw_window, KeyListener::keyCallback);
}


void Window::initializeWindow() {
    /* Init GLFW */
    if( !glfwInit() ) exit( EXIT_FAILURE );

    setUpWindowHints();

    mp_glfw_window = glfwCreateWindow( m_width, m_height, mp_title, NULL, NULL );
    glfwMakeContextCurrent(mp_glfw_window);
    glfwSwapInterval(1);
    if (!mp_glfw_window)
    {
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    setUpCallBack();
    
    /* Initialize Glew. Must be done after glfw is initialized!*/
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return ;
    }
}


void Window::mainLoop() const {
    std::cout << "Drawing our triangle" << std::endl;
    
    // Initializing frame time
    Time::update();
    while (!glfwWindowShouldClose(mp_glfw_window))
    {
        drawTriangle(mp_glfw_window);
        glfwPollEvents();
        Time::update();
        std::cout << (1.0f / Time::deltaTime()) << " FPS" << std::endl;
    }

    glfwTerminate();
    exit( EXIT_SUCCESS );
}


Window* Window::getWindow() {
    if (mp_window == nullptr) {
        mp_window = new Window();
    }

    return mp_window;
};


void Window::run() {
    initializeWindow();
    mainLoop();
}
