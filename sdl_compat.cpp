#include "sdl_compat.h"

// Global renderer pointer - initialized in Game::init_graphics()
SDL_Renderer* g_renderer = nullptr;

// Global trippy/theme state - updated in Game::update_game()
float g_hueShift = 0.0f;
int g_trippyLevel = 0;
int g_colorTheme = 0;
int g_colorThemePrev = 0;
float g_colorThemeBlend = 1.0f;
int g_tracerLength = 0;

// Camera state - updated by Game::update_camera()
float g_camera_x = 0.0f;
float g_camera_y = 0.0f;
float g_camera_zoom = 1.0f;
float g_screen_cx = 0.0f;
float g_screen_cy = 0.0f;

