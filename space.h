#ifndef _SPACE_H_
#define _SPACE_H_

#include <vector>
#include <list>
#include <map>

#include "body.h"
#include "loot.h"
#include "duder.h"
#include "geom.h"
#include "asteroid.h"
#include "cuzer.h"

using namespace std;

enum class ColorTheme {
    NORMAL,
    WARM,
    COOL,
    NEON,
    MONO
};

struct SpaceTraits {
    int numBodies;
    int numFuel;
    int numBoost;
    int numMushroom;
    int numDuders;
    int numAsteroids;
    int numCuzers;
    int numGravityWells;
    float asteroidSpeed;
    float cuzerSpeed;
    ColorTheme theme;
};

// Generate traits for a chunk at (cx, cy) with given difficulty
SpaceTraits generateTraitsForChunk(int cx, int cy, int difficulty);

// A loaded chunk of the world
struct Chunk {
    int cx, cy;
    SpaceTraits traits;
    // Entity tracking for cleanup — stores pointers into global lists
    vector<Body*> bodies;
    // We track iterators would be fragile, so we tag entities with chunk coords instead
};

#endif
