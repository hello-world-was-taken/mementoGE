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
    GLenum glewInitResult;

   /* Init GLFW */
   if( !glfwInit() )
      exit( EXIT_FAILURE );

   window = glfwCreateWindow( 400, 400, "Boing (classic Amiga demo)", NULL, NULL );

   if (!window)
   {
       glfwTerminate();
       exit( EXIT_FAILURE );
   }

    /* Init Glew*/
    glewInitResult = glewInit();
    if (glewInitResult != GLEW_OK) {
        std::cout << "Error initializing glew" << std::endl;
    }

   glfwSetWindowAspectRatio(window, 1, 1);

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