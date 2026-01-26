

#include <iostream>
#include "defines.h"
#include "body.h"
#include "space.h"
#include "sdl_compat.h"

using namespace std;

Space::Space(int coordx, int coordy, int window_w, int window_h)
:body_count(0), coordx(coordx), coordy(coordy), window_width(window_w), window_height(window_h), gravitate_bodies(false){

}

SpaceTraits Space::generateTraits(int coordx, int coordy, int difficulty) {
    SpaceTraits t;

    // Distance from origin affects trait generation
    int distance = abs(coordx) + abs(coordy);

    // Base body count: 1-10, increases slightly with difficulty
    t.numBodies = 1 + rand() % (BODY_COUNT + difficulty/2);

    // Loot count: decreases with difficulty
    int maxLoot = std::max(1, 10 - difficulty);
    t.numFuel = rand() % maxLoot;
    t.numBoost = rand() % (maxLoot / 2 + 1);

    // Duders: 0-3
    t.numDuders = rand() % 4;

    // Asteroids: more likely and more numerous at higher coordy
    if (coordy > 0) {
        t.numAsteroids = rand() % 4 + (coordy > 3 ? 2 : 0);
    } else {
        t.numAsteroids = 0;
    }

    // Cuzers: appear at coordy > 1, 0-3 count
    if (coordy > 1) {
        t.numCuzers = rand() % 4;
    } else {
        t.numCuzers = 0;
    }

    // Cuzer speed multiplier: 1.0 base, increases with distance
    t.cuzerSpeed = 1.0f + (distance * 0.08f);

    // Gravity wells: 0-2 chance, higher at distance, not at Earth level (coordy=0)
    if (coordy != 0) {
        int gravityChance = std::min(30, 5 + distance * 3);  // 5-30% chance
        t.numGravityWells = (rand() % 100 < gravityChance) ? (1 + rand() % 2) : 0;
    } else {
        t.numGravityWells = 0;
    }

    // Asteroid speed multiplier: 1.0 base, increases with distance
    t.asteroidSpeed = 1.0f + (distance * 0.1f);

    // Trippy level: 0 normally, 15-25% chance far from origin
    t.trippyLevel = 0;
    if (distance >= 3) {
        int trippyChance = std::min(25, 10 + distance * 2);  // 10-25% chance
        if (rand() % 100 < trippyChance) {
            t.trippyLevel = 1 + rand() % 3;  // 1-3 intensity
            // Trippy spaces tend to have more asteroids
            t.numAsteroids += rand() % 2;
        }
    }

    // Color theme selection
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

    // Trippy spaces more likely to be NEON
    if (t.trippyLevel > 0 && rand() % 100 < 50) {
        t.theme = ColorTheme::NEON;
    }

    // Tracer length: 0 normally, chance increases with distance
    // Range: 5-100 (5 = short trails, 100 = very long trails)
    t.tracerLength = 0;
    if (distance >= 1) {
        int tracerChance = std::min(40, 10 + distance * 5);  // 10-40% chance
        if (rand() % 100 < tracerChance) {
            t.tracerLength = 5 + rand() % 96;  // 5-100 trail length
        }
    }
    // Trippy spaces often have tracers
    if (t.trippyLevel > 0 && t.tracerLength == 0 && rand() % 100 < 70) {
        t.tracerLength = 10 + rand() % 60;  // 10-69 trail length
    }

    return t;
}

Point Space::getStartPos(float width, float height){
    float posx = rand() % (int)(window_width - width/2);
    float posy = rand() % (int)(window_height - height/2);
    if (posx - width/2 < 0) posx += width/2;
    if (posy - height/2 < 0) posy += height/2;
    return Point(posx, posy);
}

void Space::init(int difficulty){
    // Generate traits for this space
    traits = generateTraits(coordx, coordy, difficulty);

    // Use traits for body count
    body_count = traits.numBodies;
    bodies = new Body* [body_count];

    int max_size = MAX_BODY_SIZE;
    int min_size = MIN_BODY_SIZE;
    float width = min_size, height = min_size;
    Point pos;

    // Track how many gravity wells we've created
    int gravityWellsCreated = 0;

    for (int i=0; i < body_count; i++){
        width = min_size + rand()%max_size;
        height = min_size + rand()%max_size;
        pos = getStartPos(width, height);

        bodies[i] = new Body(
            pos.x, pos.y,
            width, height,
            map_rgb(rand()%255, rand()%255, rand()%255),
            rand()%2 > 0 ? true : false
        );
        bodies[i]->computeRect();

        // Create gravity wells based on traits
        if (gravityWellsCreated < traits.numGravityWells && coordy != 0) {
            // Distribute gravity wells evenly among bodies
            int interval = body_count / (traits.numGravityWells + 1);
            if (i > 0 && (i % interval == 0 || i == body_count - 1)) {
                bodies[i]->isGravityWell = true;
                float size = (bodies[i]->width + bodies[i]->height) / 2;
                bodies[i]->gravityStrength = size * 0.5f;
                float avgSize = (width + height) / 2;
                bodies[i]->width = avgSize;
                bodies[i]->height = avgSize;
                bodies[i]->width2 = avgSize / 2;
                bodies[i]->height2 = avgSize / 2;
                bodies[i]->computeRect();
                gravityWellsCreated++;
            }
        }
    }

    // make one body ground for lowest coordy
    if (coordy == 0){
        bodies[0]->pos.x = window_width/2;
        bodies[0]->pos.y = window_height - EARTH_HEIGHT/2;
        bodies[0]->width = window_width;
        bodies[0]->height = EARTH_HEIGHT;
        bodies[0]->width2 = bodies[0]->width/2;
        bodies[0]->height2 = bodies[0]->height/2;
        bodies[0]->filled = true;
        bodies[0]->round = 1;
        bodies[0]->computeRect();
    }

    // Create fuel pickups based on traits
    for (int i = 0; i < traits.numFuel; i++){
        pos = getStartPos(width, height);
        float fuel_value = 100 + rand() % (int)FUEL_START/2;
        float fuel_scale = (fuel_value - 100) / 5000.0f;
        width = 15 + fuel_scale * 20;
        height = 22 + fuel_scale * 30;
        Loot loot = Loot(pos.x, pos.y, width, height, map_rgb(255, 20, 20), FUEL);
        loot.value = fuel_value;
        loots.push_back(loot);
    }

    // Create boost pickups based on traits
    for (int i = 0; i < traits.numBoost; i++){
        pos = getStartPos(width, height);
        float boost_value = rand() % 5 + 2;
        float boost_scale = (boost_value - 2) / 4.0f;
        width = 25 + boost_scale * 30;
        height = 25 + boost_scale * 30;
        Loot loot = Loot(pos.x, pos.y, width, height, map_rgb(255, 200, 20), BOOST);
        loot.value = boost_value;
        loots.push_back(loot);
    }

    // Create duders based on traits
    for (int i = 0; i < traits.numDuders; i++){
        pos = getStartPos(width, height);
        Duder duder = Duder(pos.x, pos.y, rand()%20 + 20, rand()%15 + 15);
        duders.push_back(duder);
    }

    // Create asteroids based on traits
    for (int i = 0; i < traits.numAsteroids; i++) {
        // Random size distribution: 50% small, 35% medium, 15% large
        AsteroidSize sz;
        int sizeRoll = rand() % 100;
        if (sizeRoll < 50) {
            sz = AsteroidSize::SMALL;
        } else if (sizeRoll < 85) {
            sz = AsteroidSize::MEDIUM;
        } else {
            sz = AsteroidSize::LARGE;
        }

        pos = getStartPos(100, 100);
        Asteroid asteroid(pos.x, pos.y, sz);

        // Apply asteroid speed multiplier from traits
        float angle = (rand() % 360) * M_PI / 180.0f;
        float speed = (5.0f + (rand() % 400) / 10.0f) * traits.asteroidSpeed;
        asteroid.vel.x = cos(angle) * speed;
        asteroid.vel.y = sin(angle) * speed;

        asteroids.push_back(asteroid);
    }

    // Create cuzers based on traits
    for (int i = 0; i < traits.numCuzers; i++) {
        // Size distribution: 40% small, 40% medium, 20% large
        CuzerSize sz;
        int sizeRoll = rand() % 100;
        if (sizeRoll < 40) {
            sz = CuzerSize::SMALL;
        } else if (sizeRoll < 80) {
            sz = CuzerSize::MEDIUM;
        } else {
            sz = CuzerSize::LARGE;
        }

        pos = getStartPos(100, 100);
        Cuzer cuzer(pos.x, pos.y, sz);

        // Apply cuzer speed multiplier from traits
        float angle = (rand() % 360) * M_PI / 180.0f;
        float speed = (2.0f + (rand() % 300) / 10.0f) * traits.cuzerSpeed;
        cuzer.vel.x = cos(angle) * speed;
        cuzer.vel.y = sin(angle) * speed;

        cuzers.push_back(cuzer);
    }
}

void Space::draw(void){
    for (int i=body_count-1; i >=0 ; i--)
        bodies[i]->draw();

    for (auto& loot : loots)
        loot.draw();

    for (auto& duder : duders)
        duder.draw();

    for (auto& asteroid : asteroids)
        asteroid.draw();

    for (auto& cuzer : cuzers)
        cuzer.draw();
}
