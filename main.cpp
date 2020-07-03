#include "game.h"
#include <time.h> 
#include <iostream>
#include <string>
#include <stdlib.h>

using namespace std;


float get_parameter(string display, int max){
    float val = -1;
    string line;
    while (val < 0){
        cout << display;
        getline(cin, line);
        try{
            val = stof(line);
        }catch(invalid_argument){val = -1;}
        if (val > max) val = -1;
    }
    return val;
}

int get_difficulty(){
    return (int)get_parameter("Enter difficulty from 0 (easy) to 10 (most difficult): ", 10);
}

bool get_music_on(){
    return (bool)get_parameter("Jams on(1) or off(0)?: ", 1);
}

float get_fullscreen(){
    return get_parameter("Screen ratio? I mean if fullscreen is 1 then 0 is byeee: ", 1);
}

int main(int argc, char* argv[])
{
    Game game;

    srand(time(NULL));
    game.difficulty = get_difficulty();
    game.music_on = get_music_on();
    game.fullscreen = get_fullscreen();
    game.init_graphics();
    game.init_game();
    game.loop();
    game.shutdown();
}

