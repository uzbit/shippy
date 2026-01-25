#include "sdl_compat.h"

// Global renderer pointer - initialized in Game::init_graphics()
SDL_Renderer* g_renderer = nullptr;

// Global trippy/theme state - updated in Game::update_game()
float g_hueShift = 0.0f;
int g_trippyLevel = 0;
int g_colorTheme = 0;
int g_tracerLength = 0;
