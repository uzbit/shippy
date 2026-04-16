#ifndef _PLATFORM_H_
#define _PLATFORM_H_

#include <SDL3/SDL.h>
#include <string>

// Returns platform-appropriate path to an asset file in the data/ directory.
// On Android: returns "data/<filename>" (SDL_IOFromFile reads from APK assets/).
// On desktop: prepends SDL_GetBasePath() so it works regardless of cwd.
std::string asset_path(const char* filename);

// Read an entire text file into a string using SDL_IOFromFile.
// Works on all platforms including Android APK assets.
std::string read_text_asset(const char* filename);

#endif
