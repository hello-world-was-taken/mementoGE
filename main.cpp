#include "./Window.h"

void open_window();

int main()
{
    std::cout << "OpenGL Playaround " << std::endl;
    Window* window = Window::getWindow();
    window->run();
}