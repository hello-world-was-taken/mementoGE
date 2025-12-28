#include "log_error.h"

#include <AL/al.h>
#include <AL/alc.h>

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

void checkAlcError(ALCdevice *device, const char *msg)
{
    ALenum err = alcGetError(device);
    if (err != ALC_NO_ERROR)
    {
        std::cerr << "[OpenAL al] " << msg << " : " << alGetString(err) << "\n";
    }
}

void checkAlError(const char *msg)
{
    ALenum err = alGetError();
    if (err != AL_NO_ERROR)
    {
        std::cerr << "[OpenAL al] " << msg << " : " << alGetString(err) << "\n";
    }
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