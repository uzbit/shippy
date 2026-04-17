cmake_minimum_required(VERSION 3.14)
project(shippy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Fetch Box2D
include(FetchContent)
FetchContent_Declare(
    box2d
    GIT_REPOSITORY https://github.com/erincatto/box2d.git
    GIT_TAG v3.1.1
)
FetchContent_MakeAvailable(box2d)

if(ANDROID)
    # On Android, fetch SDL3 libraries from source
    FetchContent_Declare(
        SDL3
        GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
        GIT_TAG release-3.4.4
        GIT_SHALLOW TRUE
    )
    FetchContent_Declare(
        SDL3_ttf
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
        GIT_TAG release-3.2.2
        GIT_SHALLOW TRUE
    )
    FetchContent_Declare(
        SDL3_mixer
        GIT_REPOSITORY https://github.com/libsdl-org/SDL_mixer.git
        GIT_TAG release-3.2.0
        GIT_SHALLOW TRUE
    )
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
    set(SDL_STATIC OFF CACHE BOOL "" FORCE)

    # Only need OGG/Vorbis for our audio - disable everything else
    set(SDLMIXER_MP3 OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MOD OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_MIDI OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_FLAC OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_OPUS OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_WAVPACK OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_GME OFF CACHE BOOL "" FORCE)
    set(SDLMIXER_VORBIS_STB ON CACHE BOOL "" FORCE)

    FetchContent_MakeAvailable(SDL3 SDL3_ttf SDL3_mixer)
else()
    # On desktop, use system-installed SDL3
    find_package(SDL3 REQUIRED)
    find_package(SDL3_ttf REQUIRED)
    find_package(SDL3_mixer REQUIRED)
endif()

set(SOURCES
    main.cpp
    object.cpp
    body.cpp
    loot.cpp
    game.cpp
    ship.cpp
    space.cpp
    biases.cpp
    duder.cpp
    starfield.cpp
    sdl_compat.cpp
    physics_world.cpp
    projectile.cpp
    asteroid.cpp
    cuzer.cpp
    ai.cpp
    platform.cpp
    touch_input.cpp
    synth.cpp
)

if(ANDROID)
    add_library(shippy SHARED ${SOURCES})
    target_link_libraries(shippy
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
        SDL3_mixer::SDL3_mixer
        box2d
        log
    )
else()
    add_executable(shippy ${SOURCES})
    target_link_libraries(shippy
        SDL3::SDL3
        SDL3_ttf::SDL3_ttf
        SDL3_mixer::SDL3_mixer
        box2d
    )
endif()
