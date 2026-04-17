
#include <iostream>
#include <cmath>
#include "defines.h"
#include "space.h"
#include "sdl_compat.h"

using namespace std;

SpaceTraits generateTraitsForChunk(int cx, int cy, int difficulty) {
    SpaceTraits t;

    int distance = abs(cx) + abs(cy);

    t.numBodies = 1 + rand() % (BODY_COUNT + difficulty/2);

    int maxLoot = std::max(1, 10 - difficulty);
    t.numFuel = rand() % maxLoot;
    t.numBoost = rand() % (maxLoot / 2 + 1);
    t.numMushroom = rand() % (maxLoot / 3 + 1);

    t.numDuders = rand() % 4;

    if (abs(cy) > 0) {
        t.numAsteroids = rand() % 4 + (abs(cy) > 3 ? 2 : 0);
    } else {
        t.numAsteroids = 0;
    }

    if (abs(cy) > 1) {
        t.numCuzers = rand() % 4;
    } else {
        t.numCuzers = 0;
    }

    t.cuzerSpeed = 1.0f + (distance * 0.08f);

    if (cy != 0) {
        int gravityChance = std::min(15, 3 + distance * 1);
        t.numGravityWells = (rand() % 100 < gravityChance) ? 1 : 0;
    } else {
        t.numGravityWells = 0;
    }

    t.asteroidSpeed = 1.0f + (distance * 0.1f);

    int themeRoll = rand() % 100;
    if (themeRoll < 50) {
        t.theme = ColorTheme::NORMAL;
    } else if (themeRoll < 65) {
        t.theme = ColorTheme::WARM;
    } else if (themeRoll < 80) {
        t.theme = ColorTheme::COOL;
    } else if (themeRoll < 92) {
        t.theme = ColorTheme::NEON;
    } else {
        t.theme = ColorTheme::MONO;
    }

    return t;
}
