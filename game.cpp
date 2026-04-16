#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <cstring>

#include "defines.h"
#include "game.h"
#include "space.h"
#include "ship.h"
#include "loot.h"
#include "body.h"
#include "starfield.h"
#include "sdl_compat.h"
#include "platform.h"

using namespace std;

Game::Game()
:done(false), difficulty(1),
 window(nullptr), renderer(nullptr), font(nullptr), mixer(nullptr),
 music_audio(nullptr), music_track(nullptr), buffer(nullptr), trailBuffer(nullptr), lastFireTime(0), fireRate(200),
 state(STATE_MENU), menu_selection(2), pause_selection(0), ship(nullptr),
 chunk_w(0), chunk_h(0),
 camera_x(0), camera_y(0), camera_zoom(1.0f), camera_target_zoom(1.0f),
 camera_target_x(0), camera_target_y(0){
}

Game::~Game(){
    if (ship) {
        delete ship;
    }
    for (auto* body : world_bodies) delete body;
    world_bodies.clear();
    world_asteroids.clear();
    world_cuzers.clear();
    world_loots.clear();
    world_duders.clear();
    projectiles.clear();
    loaded_chunks.clear();
    physicsWorld.destroy();
}

void Game::init_graphics(void){
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO))
        abort("Failed to initialize SDL");

    // Get display bounds for window sizing
    SDL_DisplayID display_id = SDL_GetPrimaryDisplay();
    const SDL_DisplayMode* mode = SDL_GetCurrentDisplayMode(display_id);
#ifdef SDL_PLATFORM_ANDROID
    if (mode) {
        window_width = mode->w;
        window_height = mode->h;
    } else {
        window_width = 1920;
        window_height = 1080;
    }
#else
    if (mode) {
        window_width = (int)(mode->w * fullscreen);
        window_height = (int)(mode->h * fullscreen);
    } else {
        window_width = 1280;
        window_height = 720;
    }
#endif

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

    // Create window
#ifdef SDL_PLATFORM_ANDROID
    Uint32 window_flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#else
    Uint32 window_flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
#endif
    window = SDL_CreateWindow("Shippy", window_width, window_height, window_flags);
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
    std::string font_path = asset_path("DejaVuSans.ttf");
    font = TTF_OpenFont(font_path.c_str(), 24);
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

    // Init starfield here so it's available for the menu background
    starfield.init(window_width, window_height);

    // Init camera screen center
    g_screen_cx = window_width / 2.0f;
    g_screen_cy = window_height / 2.0f;
    g_camera_x = g_screen_cx;
    g_camera_y = g_screen_cy;
    g_camera_zoom = 1.0f;

    // Init touch input zones
    touchInput.init(window_width, window_height);
}

void Game::init_game(void){
    // Clean up previous game state
    if (ship) { delete ship; ship = nullptr; }
    for (auto* body : world_bodies) {
        body->destroyPhysics(physicsWorld);
        delete body;
    }
    world_bodies.clear();
    for (auto& a : world_asteroids) a.destroyPhysics(physicsWorld);
    world_asteroids.clear();
    for (auto& c : world_cuzers) c.destroyPhysics(physicsWorld);
    world_cuzers.clear();
    for (auto& l : world_loots) l.destroyPhysics(physicsWorld);
    world_loots.clear();
    for (auto& d : world_duders) d.destroyPhysics(physicsWorld);
    world_duders.clear();
    for (auto& p : projectiles) p.destroyPhysics(physicsWorld);
    projectiles.clear();
    loaded_chunks.clear();

    biases_groked.clear();
    lastFireTime = 0;
    camera_zoom = 1.0f;
    camera_target_zoom = 1.0f;
    chunk_w = window_width;
    chunk_h = window_height;

    // Clean up previous music
    if (music_track) { MIX_DestroyTrack(music_track); music_track = nullptr; }
    if (music_audio) { MIX_DestroyAudio(music_audio); music_audio = nullptr; }

    if (music_on) {
        std::string music_path = asset_path("Power_Glove-Clutch.ogg");
        music_audio = MIX_LoadAudio(mixer, music_path.c_str(), true);
        if (music_audio) {
            music_track = MIX_CreateTrack(mixer);
            if (music_track) {
                MIX_SetTrackAudio(music_track, music_audio);
                SDL_PropertiesID props = SDL_CreateProperties();
                SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
                MIX_PlayTrack(music_track, props);
                SDL_DestroyProperties(props);
            }
        }
    }

    physicsWorld.init(0.6f);

    // Ship starts at world origin
    ship = new Ship(chunk_w / 2.0f, chunk_h / 2.0f, FUEL_START, SHIP_MASS);
    ship->initPhysics(physicsWorld);

    // Load initial chunks around the ship
    update_chunks();

    biases.load();
}

// ---- Chunk management ----

void Game::load_chunk(int cx, int cy) {
    auto key = make_pair(cx, cy);
    if (loaded_chunks.count(key)) return;  // Already loaded

    // Seed RNG deterministically for this chunk
    unsigned int saved = rand();
    srand((unsigned int)(cx * 73856093u) ^ (unsigned int)(cy * 19349663u) ^ (unsigned int)(difficulty * 83492791u));

    SpaceTraits traits = generateTraitsForChunk(cx, cy, difficulty);

    Chunk chunk;
    chunk.cx = cx;
    chunk.cy = cy;
    chunk.traits = traits;

    float base_x = cx * chunk_w;
    float base_y = cy * chunk_h;

    // Spawn bodies
    for (int i = 0; i < traits.numBodies; i++) {
        float w = MIN_BODY_SIZE + rand() % MAX_BODY_SIZE;
        float h = MIN_BODY_SIZE + rand() % MAX_BODY_SIZE;
        float px = base_x + (rand() % (int)(chunk_w - w/2));
        float py = base_y + (rand() % (int)(chunk_h - h/2));

        Body* body = new Body(px, py, w, h,
            map_rgb(rand()%255, rand()%255, rand()%255),
            rand()%2 > 0);
        body->chunk_cx = cx;
        body->chunk_cy = cy;
        body->computeRect();
        body->initPhysics(physicsWorld);

        // Gravity wells
        if (i > 0 && traits.numGravityWells > 0) {
            int interval = traits.numBodies / (traits.numGravityWells + 1);
            if (interval > 0 && i % interval == 0) {
                body->isGravityWell = true;
                float avgSize = (w + h) / 2;
                body->gravityStrength = avgSize * 0.5f;
                body->width = avgSize;
                body->height = avgSize;
                body->width2 = avgSize / 2;
                body->height2 = avgSize / 2;
                body->computeRect();
            }
        }

        chunk.bodies.push_back(body);
        world_bodies.push_back(body);
    }

    // Spawn loot — push first, then init physics on the list element
    for (int i = 0; i < traits.numFuel; i++) {
        float fuel_value = 100 + rand() % (int)FUEL_START/2;
        float fuel_scale = (fuel_value - 100) / 5000.0f;
        float w = 15 + fuel_scale * 20;
        float h = 22 + fuel_scale * 30;
        float px = base_x + rand() % chunk_w;
        float py = base_y + rand() % chunk_h;
        world_loots.emplace_back(px, py, w, h, map_rgb(255, 20, 20), FUEL);
        Loot& loot = world_loots.back();
        loot.value = fuel_value;
        loot.chunk_cx = cx;
        loot.chunk_cy = cy;
        loot.initPhysics(physicsWorld);
    }
    for (int i = 0; i < traits.numBoost; i++) {
        float boost_value = rand() % 5 + 2;
        float boost_scale = (boost_value - 2) / 4.0f;
        float w = 25 + boost_scale * 30;
        float h = w;
        float px = base_x + rand() % chunk_w;
        float py = base_y + rand() % chunk_h;
        world_loots.emplace_back(px, py, w, h, map_rgb(255, 200, 20), BOOST);
        Loot& loot = world_loots.back();
        loot.value = boost_value;
        loot.chunk_cx = cx;
        loot.chunk_cy = cy;
        loot.initPhysics(physicsWorld);
    }

    // Spawn duders
    for (int i = 0; i < traits.numDuders; i++) {
        float px = base_x + rand() % chunk_w;
        float py = base_y + rand() % chunk_h;
        world_duders.emplace_back(px, py, rand()%20 + 20, rand()%15 + 15);
        Duder& duder = world_duders.back();
        duder.chunk_cx = cx;
        duder.chunk_cy = cy;
        duder.initPhysics(physicsWorld);
    }

    // Spawn asteroids
    for (int i = 0; i < traits.numAsteroids; i++) {
        AsteroidSize sz;
        int sizeRoll = rand() % 100;
        if (sizeRoll < 50) sz = AsteroidSize::SMALL;
        else if (sizeRoll < 85) sz = AsteroidSize::MEDIUM;
        else sz = AsteroidSize::LARGE;

        float px = base_x + rand() % chunk_w;
        float py = base_y + rand() % chunk_h;
        world_asteroids.emplace_back(px, py, sz);
        Asteroid& asteroid = world_asteroids.back();
        float a = (rand() % 360) * M_PI / 180.0f;
        float speed = (5.0f + (rand() % 400) / 10.0f) * traits.asteroidSpeed;
        asteroid.vel.x = cos(a) * speed;
        asteroid.vel.y = sin(a) * speed;
        asteroid.chunk_cx = cx;
        asteroid.chunk_cy = cy;
        asteroid.initPhysics(physicsWorld);
    }

    // Spawn cuzers
    for (int i = 0; i < traits.numCuzers; i++) {
        CuzerSize sz;
        int sizeRoll = rand() % 100;
        if (sizeRoll < 40) sz = CuzerSize::SMALL;
        else if (sizeRoll < 80) sz = CuzerSize::MEDIUM;
        else sz = CuzerSize::LARGE;

        float px = base_x + rand() % chunk_w;
        float py = base_y + rand() % chunk_h;
        world_cuzers.emplace_back(px, py, sz);
        Cuzer& cuzer = world_cuzers.back();
        float a = (rand() % 360) * M_PI / 180.0f;
        float speed = (2.0f + (rand() % 300) / 10.0f) * traits.cuzerSpeed;
        cuzer.vel.x = cos(a) * speed;
        cuzer.vel.y = sin(a) * speed;
        cuzer.chunk_cx = cx;
        cuzer.chunk_cy = cy;
        cuzer.initPhysics(physicsWorld);
    }

    loaded_chunks[key] = chunk;
    srand(saved);  // Restore RNG
}

void Game::unload_chunk(int cx, int cy) {
    auto key = make_pair(cx, cy);
    auto it = loaded_chunks.find(key);
    if (it == loaded_chunks.end()) return;

    // Remove bodies
    for (Body* body : it->second.bodies) {
        body->destroyPhysics(physicsWorld);
        world_bodies.remove(body);
        delete body;
    }

    // Remove entities belonging to this chunk
    for (auto eit = world_loots.begin(); eit != world_loots.end(); ) {
        if (eit->chunk_cx == cx && eit->chunk_cy == cy) {
            eit->destroyPhysics(physicsWorld);
            eit = world_loots.erase(eit);
        } else ++eit;
    }
    for (auto eit = world_duders.begin(); eit != world_duders.end(); ) {
        if (eit->chunk_cx == cx && eit->chunk_cy == cy) {
            eit->destroyPhysics(physicsWorld);
            eit = world_duders.erase(eit);
        } else ++eit;
    }
    for (auto eit = world_asteroids.begin(); eit != world_asteroids.end(); ) {
        if (eit->chunk_cx == cx && eit->chunk_cy == cy) {
            eit->destroyPhysics(physicsWorld);
            eit = world_asteroids.erase(eit);
        } else ++eit;
    }
    for (auto eit = world_cuzers.begin(); eit != world_cuzers.end(); ) {
        if (eit->chunk_cx == cx && eit->chunk_cy == cy) {
            eit->destroyPhysics(physicsWorld);
            eit = world_cuzers.erase(eit);
        } else ++eit;
    }

    loaded_chunks.erase(it);
}

void Game::update_chunks(void) {
    if (!ship) return;

    int ship_cx = (int)floor(ship->pos.x / chunk_w);
    int ship_cy = (int)floor(ship->pos.y / chunk_h);
    int load_radius = 2;
    int unload_radius = 3;

    // Load chunks within radius
    for (int dy = -load_radius; dy <= load_radius; dy++) {
        for (int dx = -load_radius; dx <= load_radius; dx++) {
            load_chunk(ship_cx + dx, ship_cy + dy);
        }
    }

    // Unload chunks beyond unload radius
    vector<pair<int,int>> to_unload;
    for (auto& kv : loaded_chunks) {
        int dcx = abs(kv.first.first - ship_cx);
        int dcy = abs(kv.first.second - ship_cy);
        if (dcx > unload_radius || dcy > unload_radius) {
            to_unload.push_back(kv.first);
        }
    }
    for (auto& key : to_unload) {
        unload_chunk(key.first, key.second);
    }
}

SpaceTraits& Game::get_chunk_traits(int cx, int cy) {
    auto key = make_pair(cx, cy);
    return loaded_chunks[key].traits;
}

void Game::update_graphics(void){
    // Starfield draws without camera (fixed background)
    float save_zoom = g_camera_zoom;
    float save_cx = g_camera_x, save_cy = g_camera_y;
    g_camera_zoom = 1.0f;
    g_camera_x = g_screen_cx;
    g_camera_y = g_screen_cy;
    starfield.draw();
    g_camera_zoom = save_zoom;
    g_camera_x = save_cx;
    g_camera_y = save_cy;

    // Draw all world entities (camera handles viewport)
    for (auto* body : world_bodies) body->draw();
    for (auto& loot : world_loots) loot.draw();
    for (auto& duder : world_duders) duder.draw();
    for (auto& asteroid : world_asteroids) asteroid.draw();
    for (auto& cuzer : world_cuzers) cuzer.draw();

    ship->draw();

    for (auto& proj : projectiles)
        proj.draw();

    for (auto& duder : world_duders)
        draw_duder_bias(&duder);

    // HUD draws without camera
    save_zoom = g_camera_zoom;
    save_cx = g_camera_x;
    save_cy = g_camera_y;
    g_camera_zoom = 1.0f;
    g_camera_x = g_screen_cx;
    g_camera_y = g_screen_cy;
    draw_info();
#ifdef SDL_PLATFORM_ANDROID
    touchInput.draw();
#endif
    g_camera_zoom = save_zoom;
    g_camera_x = save_cx;
    g_camera_y = save_cy;
}

void Game::update_game(void){
    update_camera();

    // Zone-based visual effects from ship's current chunk
    int ship_cx = (int)floor(ship->pos.x / chunk_w);
    int ship_cy = (int)floor(ship->pos.y / chunk_h);
    auto key = make_pair(ship_cx, ship_cy);
    if (loaded_chunks.count(key)) {
        SpaceTraits& traits = loaded_chunks[key].traits;
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
    applyGravityWells();

    // Single physics world step
    if (physicsWorld.isValid())
        physicsWorld.step(1.0f / 60.0f);

    if (ship) ship->update();
    updateProjectiles();
    updateAsteroids();
    updateCuzers();
    processCollisions();

    // Update duders
    for (auto& duder : world_duders)
        duder.update(chunk_w, chunk_h);

    // Load/unload chunks AFTER physics+collisions to avoid stale pointers
    update_chunks();
}

// update_space removed — continuous world has no space boundaries

void Game::applyGravityWells(void){
    const float G = 30.0f;
    const float maxForce = 8.0f;

    for (auto* body : world_bodies) {
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
    // Projectiles live in world coords, no space tracking needed

    // Add ship's velocity so projectiles inherit momentum
    proj.vel.x += ship->vel.x;
    proj.vel.y += ship->vel.y;

    proj.initPhysics(physicsWorld);
}

void Game::updateProjectiles(void) {
    // Update all projectiles (no wrapping — they fly freely)
    for (auto& proj : projectiles) {
        proj.update();
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
    for (auto& asteroid : world_asteroids) {
        if (!asteroid.isDestroyed())
            asteroid.update();
    }
    for (auto it = world_asteroids.begin(); it != world_asteroids.end(); ) {
        if (it->isDestroyed()) {
            it->destroyPhysics(physicsWorld);
            it = world_asteroids.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::updateCuzers(void) {
    for (auto& cuzer : world_cuzers) {
        if (!cuzer.isDestroyed())
            cuzer.update();
    }
    for (auto it = world_cuzers.begin(); it != world_cuzers.end(); ) {
        if (it->isDestroyed()) {
            it->destroyPhysics(physicsWorld);
            it = world_cuzers.erase(it);
        } else {
            ++it;
        }
    }
}

void Game::spawnChildAsteroids(Asteroid& parent) {
    AsteroidSize childSize;
    int childCount;

    switch (parent.getSize()) {
        case AsteroidSize::LARGE:
            childSize = AsteroidSize::MEDIUM;
            childCount = 2 + rand() % 2;
            break;
        case AsteroidSize::MEDIUM:
            childSize = AsteroidSize::SMALL;
            childCount = 2 + rand() % 2;
            break;
        case AsteroidSize::SMALL:
            return;
    }

    for (int i = 0; i < childCount; i++) {
        float offsetX = ((rand() % 40) - 20);
        float offsetY = ((rand() % 40) - 20);

        world_asteroids.emplace_back(parent.pos.x + offsetX, parent.pos.y + offsetY, childSize);
        Asteroid& child = world_asteroids.back();
        child.chunk_cx = parent.chunk_cx;
        child.chunk_cy = parent.chunk_cy;

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
        "(%.0f, %.0f) | Biases: %ld/%ld | Speed: %.1f | Fuel: %.1f%s",
        ship->pos.x, ship->pos.y, biases_groked.size(), biases.biases.size(), speed, ship->fuel, effects
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

// initSpacePhysics/enableSpacePhysics/disableSpacePhysics removed
// Each Space now manages its own PhysicsWorld via space->initPhysics()

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
            ) {
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
            ) {
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
            ) {
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

        // Duder-body collisions handled by Box2D physics
    }

    // Now safely remove collected loots after all events processed
    for (Loot* lootObj : lootsToRemove) {
        for (auto it = world_loots.begin(); it != world_loots.end(); ++it) {
            if (&(*it) == lootObj) {
                lootObj->destroyPhysics(physicsWorld);
                world_loots.erase(it);
                break;
            }
        }
    }

    // Remove destroyed asteroids
    for (Asteroid* asteroidObj : asteroidsToDestroy) {
        for (auto it = world_asteroids.begin(); it != world_asteroids.end(); ++it) {
            if (&(*it) == asteroidObj) {
                asteroidObj->destroyPhysics(physicsWorld);
                world_asteroids.erase(it);
                break;
            }
        }
    }

    // Remove destroyed cuzers
    for (Cuzer* cuzerObj : cuzersToDestroy) {
        for (auto it = world_cuzers.begin(); it != world_cuzers.end(); ++it) {
            if (&(*it) == cuzerObj) {
                cuzerObj->destroyPhysics(physicsWorld);
                world_cuzers.erase(it);
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

// ---- Camera ----

void Game::update_camera() {
    if (!ship) return;

    float lerp_speed = 0.08f;

    // Camera follows ship
    camera_target_x = ship->pos.x;
    camera_target_y = ship->pos.y;
    camera_target_zoom = 1.0f;

    // Smooth lerp toward targets
    camera_x += (camera_target_x - camera_x) * lerp_speed;
    camera_y += (camera_target_y - camera_y) * lerp_speed;
    camera_zoom += (camera_target_zoom - camera_zoom) * lerp_speed;

    if (fabsf(camera_zoom - camera_target_zoom) < 0.001f)
        camera_zoom = camera_target_zoom;

    // Update globals for draw functions
    g_camera_x = camera_x;
    g_camera_y = camera_y;
    g_camera_zoom = camera_zoom;
}

void Game::handle_input(void){
    // Keyboard controls
    const bool* keys = SDL_GetKeyboardState(NULL);

    if (keys[SDL_SCANCODE_RIGHT] || keys[SDL_SCANCODE_D])
        ship->rotate(1);
    if (keys[SDL_SCANCODE_LEFT] || keys[SDL_SCANCODE_A])
        ship->rotate(-1);
    if (keys[SDL_SCANCODE_UP] || keys[SDL_SCANCODE_W])
        ship->thrust(1);
    if (keys[SDL_SCANCODE_DOWN] || keys[SDL_SCANCODE_S])
        ship->brake(1);
    if (keys[SDL_SCANCODE_SPACE])
        fireProjectile();

    // Touch controls (active alongside keyboard)
    if (touchInput.state.joystick_x > 0)
        ship->rotate(touchInput.state.joystick_x);
    if (touchInput.state.joystick_x < 0)
        ship->rotate(touchInput.state.joystick_x);
    if (touchInput.state.joystick_y < 0)
        ship->thrust(-touchInput.state.joystick_y);
    if (touchInput.state.brake_pressed)
        ship->brake(1);
    if (touchInput.state.fire_pressed)
        fireProjectile();
}

// ---- Text helpers ----

void Game::draw_text(const char* text, float x, float y, SDL_Color color, bool center) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        float dx = center ? x - surface->w / 2.0f : x;
        SDL_FRect dst = {dx, y, (float)surface->w, (float)surface->h};
        SDL_RenderTexture(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

void Game::draw_text_scaled(const char* text, float x, float y, SDL_Color color, float scale, bool center) {
    SDL_Surface* surface = TTF_RenderText_Blended(font, text, 0, color);
    if (!surface) return;
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    if (texture) {
        float w = surface->w * scale;
        float h = surface->h * scale;
        float dx = center ? x - w / 2.0f : x;
        SDL_FRect dst = {dx, y, w, h};
        SDL_RenderTexture(renderer, texture, NULL, &dst);
        SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
}

// ---- Menu drawing ----

void Game::draw_menu(void) {
    float cx = window_width / 2.0f;
    float cy = window_height / 2.0f;

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {128, 128, 128, 255};
    SDL_Color highlight = {100, 200, 255, 255};

    GameColor border_normal = map_rgb(100, 100, 100);
    GameColor border_selected = map_rgb(100, 200, 255);

    // Title
    draw_text_scaled("S H I P P Y", cx, cy - 180, white, 3.0f, true);

    // Difficulty row
    float row_y = cy - 50;
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "<  %d  >", difficulty);
        SDL_Color col = (menu_selection == 0) ? highlight : white;
        draw_text("Difficulty:", cx - 160, row_y + 4, col, false);
        draw_text_scaled(buf, cx + 60, row_y, col, 1.2f, false);

        // Store hit rect for this row
        menu_rects[0] = {cx - 160, row_y, 320, 36};
    }

    // Music row
    row_y = cy + 10;
    {
        SDL_Color col = (menu_selection == 1) ? highlight : white;
        draw_text("Music:", cx - 160, row_y + 4, col, false);

        const char* on_text = music_on ? "[ON]" : " ON ";
        const char* off_text = music_on ? " OFF " : "[OFF]";
        SDL_Color on_col = music_on ? highlight : gray;
        SDL_Color off_col = music_on ? gray : highlight;
        if (menu_selection != 1) {
            on_col = music_on ? white : gray;
            off_col = music_on ? gray : white;
        }
        draw_text(on_text, cx + 50, row_y + 4, on_col, false);
        draw_text(off_text, cx + 120, row_y + 4, off_col, false);

        menu_rects[1] = {cx - 160, row_y, 320, 36};
    }

    // Start button
    row_y = cy + 80;
    {
        float btn_w = 200, btn_h = 50;
        float btn_x = cx - btn_w / 2;
        float btn_y = row_y;
        GameColor border = (menu_selection == 2) ? border_selected : border_normal;
        draw_rounded_rect(btn_x, btn_y, btn_x + btn_w, btn_y + btn_h, 10, 10, border, 2.0f);

        SDL_Color col = (menu_selection == 2) ? highlight : white;
        draw_text_scaled("S T A R T", cx, btn_y + 8, col, 1.2f, true);

        menu_rects[2] = {btn_x, btn_y, btn_w, btn_h};
    }

    // Nav hint
    draw_text("arrows / tap to navigate    enter to select", cx, cy + 170, gray, true);
}

void Game::draw_pause(void) {
    // Dim overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_FRect overlay = {0, 0, (float)window_width, (float)window_height};
    SDL_RenderFillRect(renderer, &overlay);

    float cx = window_width / 2.0f;
    float cy = window_height / 2.0f;

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color highlight = {100, 200, 255, 255};
    GameColor border_normal = map_rgb(100, 100, 100);
    GameColor border_selected = map_rgb(100, 200, 255);

    draw_text_scaled("PAUSED", cx, cy - 120, white, 2.0f, true);

    const char* labels[] = {"RESUME", "RESTART", "QUIT"};
    float btn_w = 200, btn_h = 44;

    for (int i = 0; i < 3; i++) {
        float btn_x = cx - btn_w / 2;
        float btn_y = cy - 40 + i * 60;
        GameColor border = (pause_selection == i) ? border_selected : border_normal;
        draw_rounded_rect(btn_x, btn_y, btn_x + btn_w, btn_y + btn_h, 8, 8, border, 2.0f);

        SDL_Color col = (pause_selection == i) ? highlight : white;
        draw_text(labels[i], cx, btn_y + 12, col, true);

        pause_rects[i] = {btn_x, btn_y, btn_w, btn_h};
    }
}

// ---- Menu event handling ----

static bool point_in_rect(float px, float py, const SDL_FRect& r) {
    return px >= r.x && px < r.x + r.w && py >= r.y && py < r.y + r.h;
}

void Game::handle_menu_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.scancode) {
            case SDL_SCANCODE_UP:
            case SDL_SCANCODE_W:
                menu_selection = (menu_selection + 2) % 3; // wrap up
                break;
            case SDL_SCANCODE_DOWN:
            case SDL_SCANCODE_S:
                menu_selection = (menu_selection + 1) % 3; // wrap down
                break;
            case SDL_SCANCODE_LEFT:
            case SDL_SCANCODE_A:
                if (menu_selection == 0 && difficulty > 0) difficulty--;
                if (menu_selection == 1) music_on = !music_on;
                break;
            case SDL_SCANCODE_RIGHT:
            case SDL_SCANCODE_D:
                if (menu_selection == 0 && difficulty < 10) difficulty++;
                if (menu_selection == 1) music_on = !music_on;
                break;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_SPACE:
                if (menu_selection == 1) {
                    music_on = !music_on;
                } else if (menu_selection == 2) {
                    init_game();
                    state = STATE_PLAYING;
                }
                break;
            case SDL_SCANCODE_ESCAPE:
                done = true;
                break;
            default:
                break;
        }
    }

    float mx = 0, my = 0;
    bool clicked = false;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        mx = event.button.x;
        my = event.button.y;
        clicked = true;
    }
    if (event.type == SDL_EVENT_FINGER_DOWN) {
        mx = event.tfinger.x * window_width;
        my = event.tfinger.y * window_height;
        clicked = true;
    }

    if (clicked) {
        for (int i = 0; i < 3; i++) {
            if (point_in_rect(mx, my, menu_rects[i])) {
                menu_selection = i;
                if (i == 0) {
                    // Cycle difficulty on tap
                    difficulty = (difficulty + 1) % 11;
                } else if (i == 1) {
                    music_on = !music_on;
                } else if (i == 2) {
                    init_game();
                    state = STATE_PLAYING;
                }
                break;
            }
        }
    }
}

void Game::handle_pause_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.scancode) {
            case SDL_SCANCODE_UP:
            case SDL_SCANCODE_W:
                pause_selection = (pause_selection + 2) % 3;
                break;
            case SDL_SCANCODE_DOWN:
            case SDL_SCANCODE_S:
                pause_selection = (pause_selection + 1) % 3;
                break;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_SPACE:
                if (pause_selection == 0) {
                    state = STATE_PLAYING;
                } else if (pause_selection == 1) {
                    init_game();
                    state = STATE_PLAYING;
                } else if (pause_selection == 2) {
                    state = STATE_MENU;
                    menu_selection = 2;
                }
                break;
            case SDL_SCANCODE_ESCAPE:
                state = STATE_PLAYING; // Escape resumes
                break;
            default:
                break;
        }
    }

    float mx = 0, my = 0;
    bool clicked = false;

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN && event.button.button == SDL_BUTTON_LEFT) {
        mx = event.button.x;
        my = event.button.y;
        clicked = true;
    }
    if (event.type == SDL_EVENT_FINGER_DOWN) {
        mx = event.tfinger.x * window_width;
        my = event.tfinger.y * window_height;
        clicked = true;
    }

    if (clicked) {
        for (int i = 0; i < 3; i++) {
            if (point_in_rect(mx, my, pause_rects[i])) {
                pause_selection = i;
                if (i == 0) {
                    state = STATE_PLAYING;
                } else if (i == 1) {
                    init_game();
                    state = STATE_PLAYING;
                } else if (i == 2) {
                    state = STATE_MENU;
                    menu_selection = 2;
                }
                break;
            }
        }
    }
}

// ---- SDL3 callback entry points ----

SDL_AppResult Game::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_QUIT) {
        done = true;
        return SDL_APP_SUCCESS;
    }

    switch (state) {
        case STATE_MENU:
            handle_menu_event(event);
            break;
        case STATE_PLAYING:
            if (event.type == SDL_EVENT_KEY_DOWN &&
                (event.key.scancode == SDL_SCANCODE_ESCAPE ||
                 event.key.scancode == SDL_SCANCODE_AC_BACK)) {
                state = STATE_PAUSED;
                pause_selection = 0;
            }
            if (event.type == SDL_EVENT_FINGER_DOWN ||
                event.type == SDL_EVENT_FINGER_MOTION ||
                event.type == SDL_EVENT_FINGER_UP) {
                touchInput.handle_event(event);
            }
            break;
        case STATE_PAUSED:
            handle_pause_event(event);
            break;
    }

    return done ? SDL_APP_SUCCESS : SDL_APP_CONTINUE;
}

SDL_AppResult Game::iterate(void) {
    if (done) return SDL_APP_SUCCESS;

    switch (state) {
        case STATE_MENU: {
            starfield.update();

            SDL_SetRenderTarget(renderer, buffer);
            SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
            SDL_RenderClear(renderer);
            starfield.draw();
            draw_menu();
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderTexture(renderer, buffer, NULL, NULL);
            SDL_RenderPresent(renderer);
            break;
        }
        case STATE_PLAYING: {
            handle_input();
            update_game();

            // Render the game frame to buffer
            if (g_tracerLength > 0) {
                Uint8 trailAlpha = 200 + ((g_tracerLength - 5) * 45) / 95;
                if (trailAlpha > 245) trailAlpha = 245;

                SDL_SetRenderTarget(renderer, buffer);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                SDL_SetTextureAlphaMod(trailBuffer, trailAlpha);
                SDL_RenderTexture(renderer, trailBuffer, NULL, NULL);
                SDL_SetTextureAlphaMod(trailBuffer, 255);
                update_graphics();
                SDL_SetRenderTarget(renderer, trailBuffer);
                SDL_SetTextureBlendMode(buffer, SDL_BLENDMODE_NONE);
                SDL_RenderTexture(renderer, buffer, NULL, NULL);
                SDL_SetTextureBlendMode(buffer, SDL_BLENDMODE_BLEND);
            } else {
                SDL_SetRenderTarget(renderer, buffer);
                SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_NONE);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
                update_graphics();
                SDL_SetRenderTarget(renderer, trailBuffer);
                SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
                SDL_RenderClear(renderer);
            }

            // Present buffer to screen
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderTexture(renderer, buffer, NULL, NULL);
            SDL_RenderPresent(renderer);
            break;
        }
        case STATE_PAUSED: {
            SDL_SetRenderTarget(renderer, NULL);
            SDL_RenderTexture(renderer, buffer, NULL, NULL);
            draw_pause();
            SDL_RenderPresent(renderer);
            break;
        }
    }

    return SDL_APP_CONTINUE;
}

void Game::abort(const char* message){
    SDL_Log("%s: %s", message, SDL_GetError());
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
