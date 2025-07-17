#include <iostream>
#include "game.h"

int main()
{
    //init srand
    std::srand(static_cast<unsigned>(time(NULL)));
    //inti game engine
    
    game Game;

    //game loop

    while (Game.running())
    {
       
        //update
        Game.update();

        //RENDER
        Game.render();
       

    }

    return 0;
}
