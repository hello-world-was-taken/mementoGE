#include "opengl/draw.h"

void open_window();

int main()
{
    std::cout << "OpenGL Playaround " << std::endl;
    open_window();
}

void open_window() {
    
    /* Init GLFW */
    if( !glfwInit() ) exit( EXIT_FAILURE );

    /* Asking for core profile. Should be after glfwInit and before creating a windo. Otherwise, it won't work.*/
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    std::cout << "Running on Mac" << std::endl;
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    GLFWwindow* window = glfwCreateWindow( 400, 400, "OpenGL Playaround", NULL, NULL );
    glfwMakeContextCurrent(window);
    if (!window)
    {
        glfwTerminate();
        exit( EXIT_FAILURE );
    }

    glfwSetWindowAspectRatio(window, 1, 1);
    
    /* Initialize Glew. Must be done after glfw is initialized!*/
    GLenum res = glewInit();
    if (res != GLEW_OK) {
        fprintf(stderr, "Error: '%s'\n", glewGetErrorString(res));
        return ;
    }

    /* Main loop */
    std::cout << "Drawing our triangle" << std::endl;
    while (!glfwWindowShouldClose(window))
    {
        drawTriangle(window);
        glfwPollEvents();
    }

    glfwTerminate();
    exit( EXIT_SUCCESS );
}