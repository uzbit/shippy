#include "game.h"
 

int main(int argc, char* argv[])
{
    Game game;

    game.init_graphics();
    game.init_game();
    game.loop();
    game.shutdown();
}