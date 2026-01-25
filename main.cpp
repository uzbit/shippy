#include "game.h"
#include <time.h>
#include <iostream>
#include <string>
#include <stdlib.h>
#include <getopt.h>

using namespace std;

// Default values
const int DEFAULT_DIFFICULTY = 0;
const bool DEFAULT_MUSIC = false;
const float DEFAULT_FULLSCREEN = 1.0f;

void print_usage(const char* program) {
    cout << "Usage: " << program << " [options]\n"
         << "Options:\n"
         << "  -d, --defaults       Use default settings (skip prompts)\n"
         << "  -D, --difficulty N   Set difficulty 0-10 (default: " << DEFAULT_DIFFICULTY << ")\n"
         << "  -m, --music          Enable music (default)\n"
         << "  -M, --no-music       Disable music\n"
         << "  -f, --fullscreen F   Set screen ratio 0.0-1.0 (default: " << DEFAULT_FULLSCREEN << ")\n"
         << "  -h, --help           Show this help message\n";
}

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

    // Command line parsing state
    bool use_defaults = false;
    bool difficulty_set = false;
    bool music_set = false;
    bool fullscreen_set = false;

    static struct option long_options[] = {
        {"defaults",   no_argument,       0, 'd'},
        {"difficulty", required_argument, 0, 'D'},
        {"music",      no_argument,       0, 'm'},
        {"no-music",   no_argument,       0, 'M'},
        {"fullscreen", required_argument, 0, 'f'},
        {"help",       no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "dD:mMf:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'd':
                use_defaults = true;
                break;
            case 'D':
                game.difficulty = atoi(optarg);
                if (game.difficulty < 0) game.difficulty = 0;
                if (game.difficulty > 10) game.difficulty = 10;
                difficulty_set = true;
                break;
            case 'm':
                game.music_on = true;
                music_set = true;
                break;
            case 'M':
                game.music_on = false;
                music_set = true;
                break;
            case 'f':
                game.fullscreen = atof(optarg);
                if (game.fullscreen < 0) game.fullscreen = 0;
                if (game.fullscreen > 1) game.fullscreen = 1;
                fullscreen_set = true;
                break;
            case 'h':
                print_usage(argv[0]);
                return 0;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }

    // Apply defaults or prompt for missing values
    if (use_defaults) {
        if (!difficulty_set) game.difficulty = DEFAULT_DIFFICULTY;
        if (!music_set) game.music_on = DEFAULT_MUSIC;
        if (!fullscreen_set) game.fullscreen = DEFAULT_FULLSCREEN;
    } else {
        if (!difficulty_set) game.difficulty = get_difficulty();
        if (!music_set) game.music_on = get_music_on();
        if (!fullscreen_set) game.fullscreen = get_fullscreen();
    }

    game.init_graphics();
    game.init_game();
    game.loop();
    game.shutdown();
}

