#ifndef _SPACE_H_
#define _SPACE_H_

#include <vector>
#include <list>

#include "body.h"
#include "loot.h"
#include "duder.h"
#include "geom.h"
#include "asteroid.h"
#include "cuzer.h"

using namespace std;

enum class ColorTheme {
    NORMAL,    // Original random colors
    WARM,      // Reds, oranges, yellows
    COOL,      // Blues, greens, purples
    NEON,      // Bright saturated colors
    MONO       // Grayscale/single hue
};

struct SpaceTraits {
    int numBodies;        // Number of obstacle bodies
    int numFuel;          // Number of fuel pickups
    int numBoost;         // Number of boost pickups
    int numDuders;        // Number of duders
    int numAsteroids;     // Number of asteroids
    int numCuzers;        // Number of cuzer enemies
    int numGravityWells;  // Number of gravity well bodies
    int trippyLevel;      // 0=none, 1+=intensity of color cycling
    int tracerLength;     // 0=none, 1-10 = trail length (higher = longer trails)
    float asteroidSpeed;  // Base asteroid speed multiplier
    float cuzerSpeed;     // Base cuzer speed multiplier
    ColorTheme theme;     // Color palette for the space
};

class Space {
    public:
    Space(){};
    Space(int coordx, int coordy, int window_w, int window_h);
    ~Space(){};

    void init(int difficulty);
    void draw(void);
    Point getStartPos(float width, float height);
    SpaceTraits generateTraits(int coordx, int coordy, int difficulty);

    Body **bodies; //should just use a vector, but we talkin bout practice.
    int body_count;
    int coordx, coordy;
    int window_width, window_height;
    list<Loot> loots;
    list<Duder> duders;
    list<Asteroid> asteroids;
    list<Cuzer> cuzers;
    bool gravitate_bodies;
    SpaceTraits traits;

};

#endif