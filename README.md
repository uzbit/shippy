Hello Humans.

Let's explore space.

## About

Shippy is a physics-rich 2D space exploration game. Pilot your ship through procedurally generated spaces, dodge asteroids, fight enemies (cuzers), collect loot, and watch out for gravity wells. The farther you travel from Earth, the harder it gets.

Built with **SDL3** for rendering and **Box2D 3.1.1** for physics simulation.

## Features

- Physics-based flight with thrust, fuel management, and collisions
- Procedurally generated spaces with distance-based difficulty scaling
- Asteroids that split into smaller fragments when destroyed
- Enemy cuzers that get faster and tougher with distance
- Projectile system with bouncing bullets
- Fuel and boost loot pickups
- Gravity wells that pull your ship in
- Color themes and visual effects (trippy hue shifting, tracer trails)

## Building

### Prerequisites

- C++17 compiler
- CMake
- [SDL3](https://github.com/libsdl-org/SDL), [SDL3_ttf](https://github.com/libsdl-org/SDL_ttf), and [SDL3_mixer](https://github.com/libsdl-org/SDL_mixer)

Box2D is fetched automatically via CMake FetchContent.

### Build

```bash
git clone https://github.com/uzbit/shippy.git
cd shippy
mkdir build && cd build
cmake ..
make
./shippy
```

## What's out there?

![alt text](https://github.com/uzbit/shippy/blob/feat_2.0/data/screen2.png)
