#include "log_error.h"

// TODO: We should update this to a MACRO
void glClearError() {
    while (glGetError() != GL_NO_ERROR);
}

bool glCheckError(const char* functionName, const char* file, int line) {
    GLenum error = glGetError();
    if (error != GL_NO_ERROR) {
        std::cerr << "OpenGL error: " << error << " in function " << functionName
                  << " at " << file << ":" << line << std::endl;
        return false;
    }
    return true;
}

void printPWD()
{
    char cwd[1024]; // Buffer to hold the current working directory
    if (getcwd(cwd, sizeof(cwd)) != nullptr)
    {
        std::cout << "Current working directory: " << cwd << std::endl;
    }
    else
    {
        std::cerr << "Failed to get current working directory." << std::endl;
    }
}