#include "game.h"
#include <time.h> 
#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;

int get_difficulty(){
    int val = -1;
    string line;
    while (val < 0){
        cout << "Enter difficulty from 0 (easy) to 10 (most difficult): ";
        getline(cin, line);
        try{
            val = stoi(line);
        }catch(invalid_argument){val = -1;}
        if (val > 10) val = -1;
    }
    return val;
}

int main(int argc, char* argv[])
{
    Game game;

    srand(time(NULL));
    game.difficulty = get_difficulty();
    game.init_graphics();
    game.init_game();
    game.loop();
    game.shutdown();
}

