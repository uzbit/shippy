cmake_minimum_required(VERSION 3.14)
project(shippy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(ALLEGRO5 REQUIRED
    allegro-5
    allegro_main-5
    allegro_primitives-5
    allegro_font-5
    allegro_ttf-5
    allegro_audio-5
    allegro_acodec-5
)

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
)

add_executable(shippy ${SOURCES})

target_include_directories(shippy PRIVATE ${ALLEGRO5_INCLUDE_DIRS})
target_link_directories(shippy PRIVATE ${ALLEGRO5_LIBRARY_DIRS})
target_link_libraries(shippy ${ALLEGRO5_LIBRARIES})
target_compile_options(shippy PRIVATE ${ALLEGRO5_CFLAGS_OTHER})
