#include <iostream>
#include "game/Application.h"

int main()
{
    std::cout << "MementoGE" << std::endl;
    Application app{true};
    app.start();
    return 0;
}