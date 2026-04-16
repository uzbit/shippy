#include "platform.h"

static bool file_exists(const std::string& path) {
    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "rb");
    if (io) {
        SDL_CloseIO(io);
        return true;
    }
    return false;
}

std::string asset_path(const char* filename) {
#ifdef SDL_PLATFORM_ANDROID
    return std::string("data/") + filename;
#else
    // Try base path first (next to executable)
    const char* base = SDL_GetBasePath();
    if (base) {
        std::string path = std::string(base) + "data/" + filename;
        if (file_exists(path)) return path;
    }
    // Fall back to relative path (running from project root)
    return std::string("data/") + filename;
#endif
}

std::string read_text_asset(const char* filename) {
    std::string path = asset_path(filename);
    SDL_IOStream* io = SDL_IOFromFile(path.c_str(), "r");
    if (!io) return "";

    Sint64 size = SDL_GetIOSize(io);
    if (size <= 0) {
        SDL_CloseIO(io);
        return "";
    }

    std::string content(size, '\0');
    SDL_ReadIO(io, &content[0], size);
    SDL_CloseIO(io);
    return content;
}
