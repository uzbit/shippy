#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "defines.h"
#include "game.h"
#include "space.h"
#include "ship.h"
#include "loot.h"
#include "body.h"
#include "starfield.h"
#include "sdl_compat.h"

using namespace std;

Game::Game()
:coordx(0), coordy(0), space_index(-1), prev_space_index(-1), done(false), difficulty(1),
 window(nullptr), renderer(nullptr), font(nullptr), mixer(nullptr),
 music_audio(nullptr), music_track(nullptr), buffer(nullptr), trailBuffer(nullptr), lastFireTime(0), fireRate(200){
}

Game::~Game(){
    if (ship) {
        delete ship;
    }
    for (auto& space : spaces){
        for (int j=0; j < space.body_count; j++){
            delete space.bodies[j];
        }
        delete space.bodies;
    }
    physicsWorld.destroy();
}

void Game::init_graphics(void){
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
        abort("Failed to initialize SDL");

    // Get display bounds for window sizing
    SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display_id);
    if (mode) {
        window_width = (int)(mode->w * fullscreen);
        window_height = (int)(mode->h * fullscreen);
    } else {
        window_width = 1280;
        window_height = 720;
    }

    // Initialize SDL_ttf
    if (!TTF_Init())
        abort("Failed to initialize SDL_ttf");

    // Initialize SDL_mixer
    if (!MIX_Init())
        abort("Failed to initialize SDL_mixer");

    SDL_AudioSpec spec;
    spec.freq = 44100;
    spec.format = SDL_AUDIO_S16;
    spec.channels = 2;
    mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec);
    if (!mixer)
        abort("Failed to create SDL_mixer device");

    // Load music
    if (music_on) {
        music_audio = MIX_LoadAudio(mixer, "./data/Power_Glove-Clutch.ogg", true);
        if (music_audio) {
            music_track = MIX_CreateTrack(mixer);
            if (music_track) {
                MIX_SetTrackAudio(music_track, music_audio);
                // Set up looping playback
                SDL_PropertiesID props = SDL_CreateProperties();
                SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1); // Loop indefinitely
                MIX_PlayTrack(music_track, props);
                SDL_DestroyProperties(props);
            }
        }
    }

    // Create window with high DPI support
    window = SDL_CreateWindow("Shippy", window_width, window_height,
                              SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (!window)
        abort("Failed to create window");

    // Create renderer with vsync
    renderer = SDL_CreateRenderer(window, NULL);
    if (!renderer)
        abort("Failed to create renderer");

    // Enable VSync
    SDL_SetRenderVSync(renderer, 1);

    // Get actual render output size (may differ from window size on HiDPI)
    SDL_GetRenderOutputSize(renderer, &window_width, &window_height);

    // Set global renderer for drawing functions
    g_renderer = renderer;

    // Load font
    font = TTF_OpenFont("data/DejaVuSans.ttf", 24);
    if (!font)
        abort("Failed to load font!");

    // Create render target texture for double buffering
    buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                               SDL_TEXTUREACCESS_TARGET,
                               window_width, window_height);
    SDL_SetTextureBlendMode(buffer, SDL_BLENDMODE_BLEND);

    // Create trail buffer for tracer effect (persistent between frames)
    trailBuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888,
                                    SDL_TEXTUREACCESS_TARGET,
                                    window_width, window_height);
    SDL_SetTextureBlendMode(trailBuffer, SDL_BLENDMODE_BLEND);
    // Clear trail buffer initially
    SDL_SetRenderTarget(renderer, trailBuffer);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_SetRenderTarget(renderer, NULL);

    done = false;
    last_frame_time = SDL_GetTicks();
}

void Game::init_game(void){
    physicsWorld.init(0.6f);

    ship = new Ship(window_width/2, window_height/2, FUEL_START, SHIP_MASS);
    ship->initPhysics(physicsWorld);

    add_space(0, 0);
    adjust_ship_position();
    enableSpacePhysics(spaces[space_index]);

    biases.load();
    starfield.init(window_width, window_height);
}

void Game::adjust_ship_position(void){
    Collision collision;
    float posx, posy;
    do {
        for (int i=0; i < spaces[space_index].body_count; i++){
            collision = ship->collides(spaces[space_index].bodies[i]);
            if (collision.collides){
                posx = rand() % (int)(window_width - ship->width);
                posy = rand() % (int)(window_height - ship->height);
                if (posx - ship->width < 0) posx += ship->width;
                if (posy - ship->height < 0) posy += ship->height;
                ship->pos.x = posx;
                ship->pos.y = posy;
                ship->computeRect();
                break;
            }
        }
    } while(collision.collides);

    if (b2Body_IsValid(ship->physicsBody)) {
        physicsWorld.setTransform(ship->physicsBody, ship->pos.x, ship->pos.y, ship->angle);
    }
}

void Game::add_space(int coordx, int coordy){
    Space space = Space(coordx, coordy, window_width, window_height);
    space.init(difficulty);
    spaces.push_back(space);
    space_index = spaces.size() - 1;

    // Create physics bodies immediately (disabled by default)
    initSpacePhysics(spaces[space_index]);
    disableSpacePhysics(spaces[space_index]);
}

void Game::update_graphics(void){
    starfield.draw();
    spaces[space_index].draw();
    ship->draw();
    // Only draw projectiles that are in the current space
    for (auto& proj : projectiles) {
        if (proj.coordx == coordx && proj.coordy == coordy)
            proj.draw();
    }
    for (auto& duder : spaces[space_index].duders)
        draw_duder_bias(&duder);
    draw_info();
}

void Game::update_game(void){
    prev_space_index = space_index;
    get_space_index();

    if (space_index < 0) {
        if (prev_space_index >= 0) {
            disableSpacePhysics(spaces[prev_space_index]);
        }
        add_space(coordx, coordy);
        enableSpacePhysics(spaces[space_index]);
        // Clear trail buffer when entering new space
        SDL_SetRenderTarget(renderer, trailBuffer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, NULL);
    } else if (space_index != prev_space_index && prev_space_index >= 0) {
        disableSpacePhysics(spaces[prev_space_index]);
        enableSpacePhysics(spaces[space_index]);
        // Clear trail buffer when changing spaces
        SDL_SetRenderTarget(renderer, trailBuffer);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        SDL_SetRenderTarget(renderer, NULL);
    }

    // Update trippy/theme state based on current space's traits
    if (space_index >= 0) {
        SpaceTraits& traits = spaces[space_index].traits;
        g_trippyLevel = traits.trippyLevel;
        g_colorTheme = static_cast<int>(traits.theme);
        g_tracerLength = traits.tracerLength;

        // Increment hue shift for trippy effect (speed based on level)
        if (g_trippyLevel > 0) {
            g_hueShift += g_trippyLevel * 2.0f;  // 2-6 degrees per frame
            if (g_hueShift >= 360.0f) g_hueShift -= 360.0f;
        } else {
            g_hueShift = 0.0f;
        }
    }

    starfield.update();
    ship->gravitate_bodies(spaces[space_index]);
    applyGravityWells();

    physicsWorld.step(1.0f / 60.0f);

    ship->update();
    updateProjectiles();
    updateAsteroids();
    updateCuzers();
    processCollisions();
    update_space();
}

void Game::update_space(void){
    bool positionChanged = false;
    float newX = ship->pos.x;
    float newY = ship->pos.y;

    if (ship->pos.x > window_width){
        newX = ship->width2;
        coordx += 1;
        positionChanged = true;
    }
    if (ship->pos.x < 0){
        newX = window_width - ship->width2;
        coordx -= 1;
        positionChanged = true;
    }
    if (ship->pos.y < 0){
        newY = window_height - ship->height2;
        coordy += 1;
        positionChanged = true;
    }
    if (ship->pos.y > window_height){
        newY = ship->height2;
        coordy -= 1;
        positionChanged = true;
    }

    if (positionChanged) {
        ship->pos.x = newX;
        ship->pos.y = newY;
        if (b2Body_IsValid(ship->physicsBody)) {
            physicsWorld.setTransform(ship->physicsBody, newX, newY, ship->angle);
            b2Vec2 vel = physicsWorld.getLinearVelocity(ship->physicsBody);
            ship->vel.x = vel.x;
            ship->vel.y = vel.y;
        }
    }

    // fix falling through Earth.
    if (ship->pos.y + ship->height2 > window_height - EARTH_HEIGHT && coordy == 0){
        ship->pos.y = window_height - EARTH_HEIGHT - ship->height2;
        ship->vel.y = 0;
        if (b2Body_IsValid(ship->physicsBody)) {
            physicsWorld.setTransform(ship->physicsBody, ship->pos.x, ship->pos.y, ship->angle);
            physicsWorld.setLinearVelocity(ship->physicsBody, ship->vel.x, 0);
        }
    }

    for (auto& duder : spaces[space_index].duders)
        duder.update(window_width, window_height);

}

int Game::get_space_index(void){
    space_index = -1;
    for (int i = 0; i < spaces.size(); i++){
        if (spaces[i].coordx == coordx && spaces[i].coordy == coordy){
            space_index = i;
            break;
        }
    }
    return space_index;
}

void Game::applyGravityWells(void){
    if (space_index < 0) return;

    Space& space = spaces[space_index];
    const float G = 30.0f;  // Gravitational constant (tuned for gameplay)
    const float maxForce = 8.0f;  // Cap force so gravity wells are escapable

    for (int i = 0; i < space.body_count; i++) {
        Body* body = space.bodies[i];
        if (!body->isGravityWell) continue;

        // Calculate direction from ship to gravity well
        float dx = body->pos.x - ship->pos.x;
        float dy = body->pos.y - ship->pos.y;
        float distSq = dx * dx + dy * dy;
        float dist = sqrt(distSq);

        // Avoid division by zero and limit force at very close distances
        if (dist < 80.0f) dist = 80.0f;

        // Calculate gravitational force: F = G * strength / r^2
        float force = G * body->gravityStrength / distSq;

        // Cap the force so gravity wells are escapable with thrust
        if (force > maxForce) force = maxForce;

        // Normalize direction and apply force
        float fx = (dx / dist) * force;
        float fy = (dy / dist) * force;

        if (physicsWorld.isValid() && b2Body_IsValid(ship->physicsBody)) {
            physicsWorld.applyForceToCenter(ship->physicsBody,
                fx * PIXELS_PER_METER * 60.0f,
                fy * PIXELS_PER_METER * 60.0f);
        } else {
            ship->accel.x += fx;
            ship->accel.y += fy;
        }
    }
}

void Game::fireProjectile(void) {
    Uint64 currentTime = SDL_GetTicks();
    if (currentTime - lastFireTime < fireRate) return;

    lastFireTime = currentTime;

    // Create projectile at ship's nose position
    float noseX = ship->pos.x + cos(ship->angle) * ship->height2;
    float noseY = ship->pos.y + sin(ship->angle) * ship->height2;

    // Projectile speed (independent base speed)
    float projectileSpeed = 80.0f;

    projectiles.emplace_back(noseX, noseY, ship->angle, projectileSpeed);
    Projectile& proj = projectiles.back();

    // Set projectile's space coordinates to current space
    proj.coordx = coordx;
    proj.coordy = coordy;

    // Add ship's velocity so projectiles inherit momentum
    proj.vel.x += ship->vel.x;
    proj.vel.y += ship->vel.y;

    proj.initPhysics(physicsWorld);
}

void Game::updateProjectiles(void) {
    // Update all projectiles
    for (auto& proj : projectiles) {
        proj.update();

        // Wrap projectiles around screen edges and update space coordinates
        if (proj.pos.x < 0) {
            proj.pos.x = window_width + proj.pos.x;
            proj.coordx -= 1;
            if (b2Body_IsValid(proj.physicsBody)) {
                physicsWorld.setTransform(proj.physicsBody, proj.pos.x, proj.pos.y, proj.angle);
            }
        }
        if (proj.pos.x > window_width) {
            proj.pos.x = proj.pos.x - window_width;
            proj.coordx += 1;
            if (b2Body_IsValid(proj.physicsBody)) {
                physicsWorld.setTransform(proj.physicsBody, proj.pos.x, proj.pos.y, proj.angle);
            }
        }
        if (proj.pos.y < 0) {
            proj.pos.y = window_height + proj.pos.y;
            proj.coordy += 1;
            if (b2Body_IsValid(proj.physicsBody)) {
                physicsWorld.setTransform(proj.physicsBody, proj.pos.x, proj.pos.y, proj.angle);
            }
        }
        if (proj.pos.y > window_height) {
            proj.pos.y = proj.pos.y - window_height;
            proj.coordy -= 1;
            if (b2Body_IsValid(proj.physicsBody)) {
                physicsWorld.setTransform(proj.physicsBody, proj.pos.x, proj.pos.y, proj.angle);
            }
        }
    }

    // Remove only expired projectiles
    for (auto it = projectiles.begin(); it != projectiles.end(); ) {
        if (it->isExpired()) {
            it->destroyPhysics(physicsWorld);
            it = projectiles.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::updateAsteroids(void) {
    if (space_index < 0) return;

    Space& space = spaces[space_index];

    // Update all asteroids
    for (auto& asteroid : space.asteroids) {
        if (!asteroid.isDestroyed()) {
            asteroid.update();

            // Wrap asteroids around screen edges
            if (asteroid.pos.x < -asteroid.getRadius()) {
                asteroid.pos.x = window_width + asteroid.getRadius();
                if (b2Body_IsValid(asteroid.physicsBody)) {
                    physicsWorld.setTransform(asteroid.physicsBody, asteroid.pos.x, asteroid.pos.y, asteroid.angle);
                }
            }
            if (asteroid.pos.x > window_width + asteroid.getRadius()) {
                asteroid.pos.x = -asteroid.getRadius();
                if (b2Body_IsValid(asteroid.physicsBody)) {
                    physicsWorld.setTransform(asteroid.physicsBody, asteroid.pos.x, asteroid.pos.y, asteroid.angle);
                }
            }
            if (asteroid.pos.y < -asteroid.getRadius()) {
                asteroid.pos.y = window_height + asteroid.getRadius();
                if (b2Body_IsValid(asteroid.physicsBody)) {
                    physicsWorld.setTransform(asteroid.physicsBody, asteroid.pos.x, asteroid.pos.y, asteroid.angle);
                }
            }
            if (asteroid.pos.y > window_height + asteroid.getRadius()) {
                asteroid.pos.y = -asteroid.getRadius();
                if (b2Body_IsValid(asteroid.physicsBody)) {
                    physicsWorld.setTransform(asteroid.physicsBody, asteroid.pos.x, asteroid.pos.y, asteroid.angle);
                }
            }
        }
    }

    // Remove destroyed asteroids
    for (auto it = space.asteroids.begin(); it != space.asteroids.end(); ) {
        if (it->isDestroyed()) {
            it->destroyPhysics(physicsWorld);
            it = space.asteroids.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::updateCuzers(void) {
    if (space_index < 0) return;

    Space& space = spaces[space_index];

    // Update all cuzers
    for (auto& cuzer : space.cuzers) {
        if (!cuzer.isDestroyed()) {
            cuzer.update();

            // Wrap cuzers around screen edges
            if (cuzer.pos.x < -cuzer.getRadius()) {
                cuzer.pos.x = window_width + cuzer.getRadius();
                if (b2Body_IsValid(cuzer.physicsBody)) {
                    physicsWorld.setTransform(cuzer.physicsBody, cuzer.pos.x, cuzer.pos.y, cuzer.angle);
                }
            }
            if (cuzer.pos.x > window_width + cuzer.getRadius()) {
                cuzer.pos.x = -cuzer.getRadius();
                if (b2Body_IsValid(cuzer.physicsBody)) {
                    physicsWorld.setTransform(cuzer.physicsBody, cuzer.pos.x, cuzer.pos.y, cuzer.angle);
                }
            }
            if (cuzer.pos.y < -cuzer.getRadius()) {
                cuzer.pos.y = window_height + cuzer.getRadius();
                if (b2Body_IsValid(cuzer.physicsBody)) {
                    physicsWorld.setTransform(cuzer.physicsBody, cuzer.pos.x, cuzer.pos.y, cuzer.angle);
                }
            }
            if (cuzer.pos.y > window_height + cuzer.getRadius()) {
                cuzer.pos.y = -cuzer.getRadius();
                if (b2Body_IsValid(cuzer.physicsBody)) {
                    physicsWorld.setTransform(cuzer.physicsBody, cuzer.pos.x, cuzer.pos.y, cuzer.angle);
                }
            }
        }
    }

    // Remove destroyed cuzers
    for (auto it = space.cuzers.begin(); it != space.cuzers.end(); ) {
        if (it->isDestroyed()) {
            it->destroyPhysics(physicsWorld);
            it = space.cuzers.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::spawnChildAsteroids(Asteroid& parent) {
    if (space_index < 0) return;

    Space& space = spaces[space_index];

    // Determine child size
    AsteroidSize childSize;
    int childCount;

    switch (parent.getSize()) {
        case AsteroidSize::LARGE:
            childSize = AsteroidSize::MEDIUM;
            childCount = 2 + rand() % 2;  // 2-3 children
            break;
        case AsteroidSize::MEDIUM:
            childSize = AsteroidSize::SMALL;
            childCount = 2 + rand() % 2;  // 2-3 children
            break;
        case AsteroidSize::SMALL:
            // Small asteroids don't spawn children
            return;
    }

    for (int i = 0; i < childCount; i++) {
        // Spawn at parent position with random offset
        float offsetX = ((rand() % 40) - 20);
        float offsetY = ((rand() % 40) - 20);

        // Create asteroid directly in list (list has stable pointers)
        space.asteroids.emplace_back(parent.pos.x + offsetX, parent.pos.y + offsetY, childSize);
        Asteroid& child = space.asteroids.back();

        // Give children some velocity away from center
        float angle = (float)i * 2.0f * M_PI / childCount + ((rand() % 100) / 100.0f);
        float speed = 2.0f + (rand() % 30) / 10.0f;
        child.vel.x = cos(angle) * speed;
        child.vel.y = sin(angle) * speed;

        child.initPhysics(physicsWorld);
    }
}

void Game::draw_info(void){
    GameColor color = map_rgb(255, 255, 255);
    float speed = sqrt(ship->vel.x*ship->vel.x + ship->vel.y*ship->vel.y);

    // Build effects string
    char effects[64] = "";
    if (g_trippyLevel > 0 || g_tracerLength > 0) {
        int pos = 0;
        pos += snprintf(effects + pos, sizeof(effects) - pos, " |");
        if (g_trippyLevel > 0) pos += snprintf(effects + pos, sizeof(effects) - pos, " Trippy:%d", g_trippyLevel);
        if (g_tracerLength > 0) pos += snprintf(effects + pos, sizeof(effects) - pos, " Tracers:%d", g_tracerLength);
    }

    char buf[512];
    snprintf(buf, sizeof(buf),
        "(%d, %d) | Spaces: %ld | Biases: %ld/%ld | Speed: %.1f | Fuel: %.1f%s",
        coordx, coordy, spaces.size(), biases_groked.size(), biases.biases.size(), speed, ship->fuel, effects
    );

    // Render text to surface, then create texture
    SDL_Color sdl_color = {255, 255, 255, 255};
    SDL_Surface* surface = TTF_RenderText_Blended(font, buf, 0, sdl_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_FRect dst = {10, 10, (float)surface->w, (float)surface->h};
            SDL_RenderTexture(renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }
    (void)color; // Suppress unused variable warning
}

void Game::initSpacePhysics(Space& space) {
    for (int i = 0; i < space.body_count; i++) {
        space.bodies[i]->initPhysics(physicsWorld);
    }
    for (auto& loot : space.loots) {
        loot.initPhysics(physicsWorld);
    }
    for (auto& duder : space.duders) {
        duder.initPhysics(physicsWorld);
    }
    for (auto& asteroid : space.asteroids) {
        asteroid.initPhysics(physicsWorld);
    }
    for (auto& cuzer : space.cuzers) {
        cuzer.initPhysics(physicsWorld);
    }
}

void Game::enableSpacePhysics(Space& space) {
    for (int i = 0; i < space.body_count; i++) {
        physicsWorld.enableBody(space.bodies[i]->physicsBody);
    }
    for (auto& loot : space.loots) {
        physicsWorld.enableBody(loot.physicsBody);
    }
    for (auto& duder : space.duders) {
        if (!duder.is_killed) {
            physicsWorld.enableBody(duder.physicsBody);
            // Restore velocity for kinematic bodies (vel is pixels/frame, convert to pixels/second)
            if (b2Body_IsValid(duder.physicsBody)) {
                physicsWorld.setLinearVelocity(duder.physicsBody, duder.vel.x * FRAME_RATE, duder.vel.y * FRAME_RATE);
            }
        }
    }
    for (auto& asteroid : space.asteroids) {
        if (!asteroid.isDestroyed()) {
            physicsWorld.enableBody(asteroid.physicsBody);
            if (b2Body_IsValid(asteroid.physicsBody)) {
                physicsWorld.setLinearVelocity(asteroid.physicsBody, asteroid.vel.x, asteroid.vel.y);
            }
        }
    }
    for (auto& cuzer : space.cuzers) {
        if (!cuzer.isDestroyed()) {
            physicsWorld.enableBody(cuzer.physicsBody);
            if (b2Body_IsValid(cuzer.physicsBody)) {
                physicsWorld.setLinearVelocity(cuzer.physicsBody, cuzer.vel.x, cuzer.vel.y);
            }
        }
    }
}

void Game::disableSpacePhysics(Space& space) {
    for (int i = 0; i < space.body_count; i++) {
        physicsWorld.disableBody(space.bodies[i]->physicsBody);
    }
    for (auto& loot : space.loots) {
        physicsWorld.disableBody(loot.physicsBody);
    }
    for (auto& duder : space.duders) {
        physicsWorld.disableBody(duder.physicsBody);
    }
    for (auto& asteroid : space.asteroids) {
        physicsWorld.disableBody(asteroid.physicsBody);
    }
    for (auto& cuzer : space.cuzers) {
        physicsWorld.disableBody(cuzer.physicsBody);
    }
}

void Game::processCollisions(void) {
    if (!physicsWorld.isValid()) return;

    // Collect items to remove (defer removal until after processing all events)
    vector<Loot*> lootsToRemove;
    vector<Asteroid*> asteroidsToDestroy;
    vector<Cuzer*> cuzersToDestroy;
    vector<Projectile*> projectilesToDestroy;

    // SENSOR events for loot pickup (sensors don't generate contact events)
    b2SensorEvents sensorEvents = b2World_GetSensorEvents(physicsWorld.getWorldId());
    for (int i = 0; i < sensorEvents.beginCount; i++) {
        b2SensorBeginTouchEvent* event = sensorEvents.beginEvents + i;

        // Check if shapes are still valid (might have been destroyed)
        if (!b2Shape_IsValid(event->sensorShapeId) || !b2Shape_IsValid(event->visitorShapeId)) {
            continue;
        }

        b2BodyId sensorBody = b2Shape_GetBody(event->sensorShapeId);
        b2BodyId visitorBody = b2Shape_GetBody(event->visitorShapeId);

        // Check if bodies are still valid
        if (!b2Body_IsValid(sensorBody) || !b2Body_IsValid(visitorBody)) {
            continue;
        }

        Object* sensorObj = (Object*)b2Body_GetUserData(sensorBody);
        Object* visitorObj = (Object*)b2Body_GetUserData(visitorBody);

        if (!sensorObj || !visitorObj) continue;

        // Check for ship + loot
        Ship* shipObj = dynamic_cast<Ship*>(visitorObj);
        Loot* lootObj = dynamic_cast<Loot*>(sensorObj);

        if (shipObj && lootObj) {
            // Check if already marked for removal
            bool alreadyMarked = false;
            for (Loot* l : lootsToRemove) {
                if (l == lootObj) {
                    alreadyMarked = true;
                    break;
                }
            }
            if (!alreadyMarked) {
                apply_loot(lootObj);
                lootsToRemove.push_back(lootObj);
            }
        }
    }

    // CONTACT events for physical collisions (ship/duder, duder/body)
    b2ContactEvents contactEvents = b2World_GetContactEvents(physicsWorld.getWorldId());
    for (int i = 0; i < contactEvents.beginCount; i++) {
        b2ContactBeginTouchEvent* beginEvent = contactEvents.beginEvents + i;

        // Check if shapes are still valid
        if (!b2Shape_IsValid(beginEvent->shapeIdA) || !b2Shape_IsValid(beginEvent->shapeIdB)) {
            continue;
        }

        Object* objA = (Object*)b2Shape_GetUserData(beginEvent->shapeIdA);
        Object* objB = (Object*)b2Shape_GetUserData(beginEvent->shapeIdB);

        if (!objA || !objB) continue;

        Ship* shipObj = dynamic_cast<Ship*>(objA);
        if (!shipObj) shipObj = dynamic_cast<Ship*>(objB);

        Duder* duderObj = dynamic_cast<Duder*>(objA);
        if (!duderObj) duderObj = dynamic_cast<Duder*>(objB);

        Body* bodyObj = dynamic_cast<Body*>(objA);
        if (!bodyObj) bodyObj = dynamic_cast<Body*>(objB);

        Asteroid* asteroidObj = dynamic_cast<Asteroid*>(objA);
        if (!asteroidObj) asteroidObj = dynamic_cast<Asteroid*>(objB);

        Projectile* projectileObj = dynamic_cast<Projectile*>(objA);
        if (!projectileObj) projectileObj = dynamic_cast<Projectile*>(objB);

        Cuzer* cuzerObj = dynamic_cast<Cuzer*>(objA);
        if (!cuzerObj) cuzerObj = dynamic_cast<Cuzer*>(objB);

        if (shipObj && duderObj && !duderObj->is_killed) {
            auto it = std::next(biases.biases.begin(), duderObj->random_val % biases.biases.size());
            duderObj->bias = &(*it);
            duderObj->is_killed = true;
            // Disable physics body so killed duder doesn't collide
            physicsWorld.disableBody(duderObj->physicsBody);
            biases_groked.insert(duderObj->bias->first);
        }

        // Projectile-asteroid collision: destroy both, spawn children
        // Only process if projectile is in current space
        if (projectileObj && asteroidObj && !asteroidObj->isDestroyed() && !projectileObj->isExpired()
            && projectileObj->coordx == coordx && projectileObj->coordy == coordy) {
            // Check if not already marked for destruction
            bool asteroidMarked = false;
            for (Asteroid* a : asteroidsToDestroy) {
                if (a == asteroidObj) { asteroidMarked = true; break; }
            }
            bool projectileMarked = false;
            for (Projectile* p : projectilesToDestroy) {
                if (p == projectileObj) { projectileMarked = true; break; }
            }

            if (!asteroidMarked) {
                spawnChildAsteroids(*asteroidObj);
                asteroidObj->destroy();
                asteroidsToDestroy.push_back(asteroidObj);
            }
            if (!projectileMarked) {
                projectileObj->expired = true;  // Immediately expire on hit
                projectilesToDestroy.push_back(projectileObj);
            }
        }

        // Projectile-duder collision: kill duder and destroy projectile
        // Only process if projectile is in current space
        if (projectileObj && duderObj && !duderObj->is_killed && !projectileObj->isExpired()
            && projectileObj->coordx == coordx && projectileObj->coordy == coordy) {
            bool projectileMarked = false;
            for (Projectile* p : projectilesToDestroy) {
                if (p == projectileObj) { projectileMarked = true; break; }
            }

            // Kill the duder and assign a random bias
            auto it = std::next(biases.biases.begin(), duderObj->random_val % biases.biases.size());
            duderObj->bias = &(*it);
            duderObj->is_killed = true;
            physicsWorld.disableBody(duderObj->physicsBody);
            biases_groked.insert(duderObj->bias->first);

            // Destroy the projectile
            if (!projectileMarked) {
                projectileObj->expired = true;
                projectilesToDestroy.push_back(projectileObj);
            }
        }

        // Ship-asteroid collision: damage ship (lose fuel)
        if (shipObj && asteroidObj && !asteroidObj->isDestroyed()) {
            // Lose fuel proportional to asteroid size
            float damage = 100.0f;
            switch (asteroidObj->getSize()) {
                case AsteroidSize::SMALL: damage = 50.0f; break;
                case AsteroidSize::MEDIUM: damage = 150.0f; break;
                case AsteroidSize::LARGE: damage = 300.0f; break;
            }
            ship->fuel -= damage;
            if (ship->fuel < 0) ship->fuel = 0;
        }

        // Projectile-cuzer collision: hit cuzer and destroy projectile
        // Only process if projectile is in current space
        if (projectileObj && cuzerObj && !cuzerObj->isDestroyed() && !projectileObj->isExpired()
            && projectileObj->coordx == coordx && projectileObj->coordy == coordy) {
            bool cuzerMarked = false;
            for (Cuzer* c : cuzersToDestroy) {
                if (c == cuzerObj) { cuzerMarked = true; break; }
            }
            bool projectileMarked = false;
            for (Projectile* p : projectilesToDestroy) {
                if (p == projectileObj) { projectileMarked = true; break; }
            }

            if (!cuzerMarked) {
                // takeHit() returns true if cuzer is destroyed
                if (cuzerObj->takeHit()) {
                    cuzersToDestroy.push_back(cuzerObj);
                }
            }
            if (!projectileMarked) {
                projectileObj->expired = true;
                projectilesToDestroy.push_back(projectileObj);
            }
        }

        // Ship-cuzer collision: damage ship (lose fuel)
        if (shipObj && cuzerObj && !cuzerObj->isDestroyed()) {
            // Lose fuel proportional to cuzer size
            float damage = 100.0f;
            switch (cuzerObj->getSize()) {
                case CuzerSize::SMALL: damage = 40.0f; break;
                case CuzerSize::MEDIUM: damage = 100.0f; break;
                case CuzerSize::LARGE: damage = 200.0f; break;
            }
            ship->fuel -= damage;
            if (ship->fuel < 0) ship->fuel = 0;
        }

        // Duder-body collisions are handled automatically by Box2D physics
        // with restitution = 1.0 for proper bouncing
    }

    // Now safely remove collected loots after all events processed
    Space* curr_space = &spaces[space_index];
    for (Loot* lootObj : lootsToRemove) {
        for (auto it = curr_space->loots.begin(); it != curr_space->loots.end(); ++it) {
            if (&(*it) == lootObj) {
                lootObj->destroyPhysics(physicsWorld);
                curr_space->loots.erase(it);
                break;
            }
        }
    }

    // Remove destroyed asteroids
    for (Asteroid* asteroidObj : asteroidsToDestroy) {
        for (auto it = curr_space->asteroids.begin(); it != curr_space->asteroids.end(); ++it) {
            if (&(*it) == asteroidObj) {
                asteroidObj->destroyPhysics(physicsWorld);
                curr_space->asteroids.erase(it);
                break;
            }
        }
    }

    // Remove destroyed cuzers
    for (Cuzer* cuzerObj : cuzersToDestroy) {
        for (auto it = curr_space->cuzers.begin(); it != curr_space->cuzers.end(); ++it) {
            if (&(*it) == cuzerObj) {
                cuzerObj->destroyPhysics(physicsWorld);
                curr_space->cuzers.erase(it);
                break;
            }
        }
    }

    // Expire projectiles that hit asteroids
    for (Projectile* projObj : projectilesToDestroy) {
        for (auto& proj : projectiles) {
            if (&proj == projObj) {
                // Mark as expired so it gets cleaned up in updateProjectiles
                if (proj.getBounceCount() >= proj.maxBounces) {
                    // Already handled
                }
                break;
            }
        }
    }
}

void Game::draw_duder_bias(Duder *duder){
    if (!duder->is_killed) return;

    char buf[500];
    memset(buf, 0, sizeof(buf));
    int countword = 0, lastbr = 0, linenum = 0;
    string txt = duder->bias->second;
    int maxwords = 7;
    int tw = 0;
    float posx, posy = duder->pos.y - 60;

    SDL_Color sdl_color = {
        (Uint8)(duder->color.r * 255),
        (Uint8)(duder->color.g * 255),
        (Uint8)(duder->color.b * 255),
        255
    };

    for (int i=0; i < txt.size(); i++){
        if (txt[i] == ' '){
            countword++;
        }
        if (countword >= maxwords){
            lastbr = i;
            linenum++;
            countword = 0;
            if (!tw){
                // Measure text width
                int text_w, text_h;
                TTF_GetStringSize(font, buf, 0, &text_w, &text_h);
                tw = text_w + 30;
                posx = duder->pos.x - tw/2;
                if (posx+tw > window_width) posx = window_width-tw;
                if (posx < 0) posx = 30;
            }
            // Render text
            SDL_Surface* surface = TTF_RenderText_Blended(font, buf, 0, sdl_color);
            if (surface) {
                SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
                if (texture) {
                    SDL_FRect dst = {posx, posy + linenum * 20, (float)surface->w, (float)surface->h};
                    SDL_RenderTexture(renderer, texture, NULL, &dst);
                    SDL_DestroyTexture(texture);
                }
                SDL_DestroySurface(surface);
            }
            memset(buf, 0, sizeof(buf));
        }

        if (i-lastbr < sizeof(buf))
            buf[i-lastbr] = txt[i];
        else
            countword = maxwords + 1;
    }
    //draw last line
    linenum++;
    SDL_Surface* surface = TTF_RenderText_Blended(font, buf, 0, sdl_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_FRect dst = {posx, posy + linenum * 20, (float)surface->w, (float)surface->h};
            SDL_RenderTexture(renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }

    snprintf(buf, sizeof(buf), "%s:", duder->bias->first.c_str());
    surface = TTF_RenderText_Blended(font, buf, 0, sdl_color);
    if (surface) {
        SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
        if (texture) {
            SDL_FRect dst = {posx, posy, (float)surface->w, (float)surface->h};
            SDL_RenderTexture(renderer, texture, NULL, &dst);
            SDL_DestroyTexture(texture);
        }
        SDL_DestroySurface(surface);
    }
}

void Game::apply_loot(Loot *loot){
    switch(loot->type){
        case FUEL:
            ship->fuel += loot->value;
            ship->fuel = min(ship->fuel_start, ship->fuel);
            break;
        case BOOST:
            ship->vel.x *= loot->value;
            ship->vel.y *= loot->value;
            if (b2Body_IsValid(ship->physicsBody)) {
                physicsWorld.setLinearVelocity(ship->physicsBody, ship->vel.x, ship->vel.y);
            }
            break;
        case NUM_LOOT:
            break;
    }
}

void Game::handle_input(void){
    const bool* keys = SDL_GetKeyboardState(NULL);

    // Rotation controls (left/right or A/D)
    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        ship->rotate(1);

    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        ship->rotate(-1);

    // Thrust forward (up or W)
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])
        ship->thrust(1);

    // Brake (down or S)
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S])
        ship->brake(1);

    // Fire projectile (Space)
    if (keys[SDL_SCANCODE_SPACE])
        fireProjectile();

    if (keys[SDL_SCANCODE_ESCAPE])
        done = true;

    if (keys[SDL_SCANCODE_N])
        init_game();
}

void Game::loop(void){
    const Uint64 frame_delay = 1000 / FRAME_RATE;

    while (!done) {
        Uint64 frame_start = SDL_GetTicks();

        // Process events
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
        }

        // Handle continuous keyboard input
        handle_input();

        // Update game logic
        update_game();

        if (g_tracerLength > 0) {
            // Tracer mode: use texture alpha modulation for fading trails
            // tracerLength 5 = alpha 200 (fast fade), 100 = alpha 245 (slow fade, long trails)
            // Keep max under 250 so trails fully fade to black over time
            Uint8 trailAlpha = 200 + ((g_tracerLength - 5) * 45) / 95;
            if (trailAlpha > 245) trailAlpha = 245;

            // Draw to main buffer: first the faded trail, then current frame
            SDL_SetRenderTarget(renderer, buffer);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Draw previous trail with faded alpha
            SDL_SetTextureAlphaMod(trailBuffer, trailAlpha);
            SDL_RenderTexture(renderer, trailBuffer, NULL, NULL);
            SDL_SetTextureAlphaMod(trailBuffer, 255);  // Reset

            // Draw current frame on top
            update_graphics();

            // Copy buffer to trailBuffer for next frame (use NONE to replace, not blend)
            SDL_SetRenderTarget(renderer, trailBuffer);
            SDL_SetTextureBlendMode(buffer, SDL_BLENDMODE_NONE);
            SDL_RenderTexture(renderer, buffer, NULL, NULL);
            SDL_SetTextureBlendMode(buffer, SDL_BLENDMODE_BLEND);  // Restore

            // Copy buffer to screen
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderTexture(renderer, buffer, NULL, NULL);
            SDL_RenderPresent(renderer);
        } else {
            // Normal mode: use buffer for clean double-buffering
            SDL_SetRenderTarget(renderer, buffer);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            update_graphics();

            // Clear trail buffer so it's fresh when entering tracer space
            SDL_SetRenderTarget(renderer, trailBuffer);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);

            // Render buffer to screen
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderTexture(renderer, buffer, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        // Frame rate limiting (VSync should handle this, but as backup)
        Uint64 frame_time = SDL_GetTicks() - frame_start;
        if (frame_time < frame_delay) {
            SDL_Delay(frame_delay - frame_time);
        }
    }
}

void Game::abort(const char* message){
    printf("%s: %s\n", message, SDL_GetError());
    shutdown();
    exit(1);
}

void Game::shutdown(void){
    if (buffer)
        SDL_DestroyTexture(buffer);

    if (trailBuffer)
        SDL_DestroyTexture(trailBuffer);

    if (font)
        TTF_CloseFont(font);

    if (music_track)
        MIX_DestroyTrack(music_track);

    if (music_audio)
        MIX_DestroyAudio(music_audio);

    if (mixer)
        MIX_DestroyMixer(mixer);

    MIX_Quit();

    if (renderer)
        SDL_DestroyRenderer(renderer);

    if (window)
        SDL_DestroyWindow(window);

    TTF_Quit();
    SDL_Quit();
}
