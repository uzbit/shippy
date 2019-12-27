#include "game.h"
#include <time.h> 
#include <stdlib.h>

int main(int argc, char* argv[])
{
    Game game;

    srand(time(NULL));

    game.init_graphics();
    game.init_game();
    game.loop();
    game.shutdown();
}