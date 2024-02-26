#include "Window.h"


int main()
{
    std::cout << "OpenGL Playaround " << std::endl;
    Window* window = Window::getWindow();
    window->run();
}