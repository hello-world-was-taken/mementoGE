#include<iostream>
#include<GL/glew.h>
#include<GLFW/glfw3.h>

void open_window();

int main()
{
    std::cout << "OpenGL Playaround " << std::endl;
    open_window();
}

void open_window() {
    GLFWwindow* window;

    /* Init GLFW */
    if( !glfwInit() ) exit( EXIT_FAILURE );

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    window = glfwCreateWindow( 400, 400, "OpenGL Playaround", NULL, NULL );
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
    for (;;)
    {
        /* Swap buffers */
        glfwSwapBuffers(window);
        glfwPollEvents();

        /* Check if we are still running */
        if (glfwWindowShouldClose(window))
            break;
    }

    glfwTerminate();
    exit( EXIT_SUCCESS );
}