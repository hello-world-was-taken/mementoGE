#include <iostream>
#include "game/Game.h"

int main()
{
    std::cout << "MementoGE" << std::endl;
    Game app{true};
    app.start();
    return 0;
}