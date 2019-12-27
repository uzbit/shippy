#include "game.h"
 

int main(int argc, char* argv[])
{
    Game game;

    game.init_graphics();
    game.loop();
    game.shutdown();
}