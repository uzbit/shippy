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

# Find SDL3 and related libraries using cmake config mode
find_package(SDL3 REQUIRED)
find_package(SDL3_ttf REQUIRED)
find_package(SDL3_mixer REQUIRED)

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
)

add_executable(shippy ${SOURCES})

target_link_libraries(shippy
    SDL3::SDL3
    SDL3_ttf::SDL3_ttf
    SDL3_mixer::SDL3_mixer
    box2d
)
