#define SDL_MAIN_USE_CALLBACKS 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <time.h>
#include <stdlib.h>
#include "game.h"
#include "pdaudio.h"

static Game* game = nullptr;

SDL_AppResult SDL_AppInit(void** appstate, int argc, char* argv[]) {
    srand(time(NULL));

    game = new Game();
    game->difficulty = 0;
    game->music_mode = Game::MUSIC_OFF;
    game->fullscreen = 1.0f;

    game->init_graphics();
    pd_init();
    *appstate = game;
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event) {
    return ((Game*)appstate)->handle_event(*event);
}

SDL_AppResult SDL_AppIterate(void* appstate) {
    return ((Game*)appstate)->iterate();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result) {
    pd_shutdown();
    if (game) {
        game->shutdown();
        delete game;
        game = nullptr;
    }
}
