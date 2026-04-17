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
#include "synth.h"
#include "body.h"
#include "starfield.h"
#include "sdl_compat.h"
#include "platform.h"

using namespace std;

Game::Game()
:done(false), difficulty(1),
 window(nullptr), renderer(nullptr), font(nullptr), mixer(nullptr),
 music_audio(nullptr), music_track(nullptr),
 buffer(nullptr), trailBuffer(nullptr), lastFireTime(0), fireRate(200),
 state(STATE_MENU), menu_selection(2), pause_selection(0), ship(nullptr),
 chunk_w(0), chunk_h(0),
 camera_x(0), camera_y(0), camera_zoom(1.0f), camera_target_zoom(1.0f),
 camera_target_x(0), camera_target_y(0){
    for (int i = 0; i < SFX_TRACK_COUNT; i++) {
        sfx_tracks[i] = nullptr;
        sfx_audios[i] = nullptr;
        sfx_dirty[i] = false;
    }
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

    // Create per-role SFX tracks for looping impact sounds
    for (int i = 0; i < SFX_TRACK_COUNT; i++) {
        sfx_tracks[i] = MIX_CreateTrack(mixer);
    }

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
    lastFireTapTime = 0;
    fireWasReleased = true;
    camera_zoom = 1.0f;
    camera_target_zoom = 1.0f;
    trippyTimer = 0.0f;
    tracerTimer = 0.0f;
    tracerTimerMax = 0.0f;
    tracerLengthMax = 0;
    g_trippyLevel = 0;
    g_tracerLength = 0;
    g_hueShift = 0.0f;
    synth_music_init(musicState);
    particles.clear();
    clearAllSfxLoops();
    chunk_w = window_width;
    chunk_h = window_height;

    // Clean up previous music
    if (music_track) { MIX_DestroyTrack(music_track); music_track = nullptr; }
    if (music_audio) { MIX_DestroyAudio(music_audio); music_audio = nullptr; }

    if (music_mode == MUSIC_ON) {
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
    // MUSIC_CUSTOM: no file music, sounds come from our synth system
    // MUSIC_OFF: no music at all

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
            map_rgb(rand()%255, rand()%255, rand()%255));
        body->chunk_cx = cx;
        body->chunk_cy = cy;
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
    for (int i = 0; i < traits.numMushroom; i++) {
        float shroom_value = 1 + rand() % 3;  // trippy level 1-3
        float shroom_scale = (shroom_value - 1) / 2.0f;
        float w = 40 + shroom_scale * 30;
        float h = w * 1.2f;
        float px = base_x + rand() % chunk_w;
        float py = base_y + rand() % chunk_h;
        world_loots.emplace_back(px, py, w, h, map_rgb(180, 50, 220), MUSHROOM);
        Loot& loot = world_loots.back();
        loot.value = shroom_value;
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
        if (eit->chunk_cx == cx && eit->chunk_cy == cy && !eit->is_following) {
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
    // Starfield handles its own parallax using camera globals
    starfield.draw();

    // Draw all world entities (camera handles viewport)
    for (auto* body : world_bodies) body->draw();
    for (auto& loot : world_loots) loot.draw();
    for (auto& duder : world_duders) duder.draw();
    for (auto& asteroid : world_asteroids) asteroid.draw();
    for (auto& cuzer : world_cuzers) cuzer.draw();

    ship->draw();

    for (auto& proj : projectiles)
        proj.draw();

    drawParticles();

    for (auto& duder : world_duders)
        draw_duder_bias(&duder);
}

void Game::draw_hud(void){
    float save_zoom = g_camera_zoom;
    float save_cx = g_camera_x, save_cy = g_camera_y;
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

    // Zone-based color theme from ship's current chunk (with smooth blending)
    int ship_cx = (int)floor(ship->pos.x / chunk_w);
    int ship_cy = (int)floor(ship->pos.y / chunk_h);
    auto key = make_pair(ship_cx, ship_cy);
    if (loaded_chunks.count(key)) {
        int newTheme = static_cast<int>(loaded_chunks[key].traits.theme);
        if (newTheme != g_colorTheme) {
            g_colorThemePrev = g_colorTheme;
            g_colorTheme = newTheme;
            g_colorThemeBlend = 0.0f;
        }
    }
    if (g_colorThemeBlend < 1.0f) {
        g_colorThemeBlend += 0.01f;  // ~1.7 second transition at 60fps
        if (g_colorThemeBlend > 1.0f) g_colorThemeBlend = 1.0f;
    }

    // Tick music state
    float dt = 1.0f / 60.0f;
    synth_music_tick(musicState, dt);

    // Decay loot effect timers
    if (trippyTimer > 0.0f) {
        trippyTimer -= dt;
        if (trippyTimer <= 0.0f) {
            trippyTimer = 0.0f;
            g_trippyLevel = 0;
        }
    }
    if (tracerTimer > 0.0f) {
        tracerTimer -= dt;
        if (tracerTimer <= 0.0f) {
            tracerTimer = 0.0f;
            g_tracerLength = 0;
        } else {
            float ratio = tracerTimer / tracerTimerMax;
            g_tracerLength = (int)(tracerLengthMax * ratio);
        }
    }

    // Increment hue shift for trippy effect (speed based on level)
    if (g_trippyLevel > 0) {
        g_hueShift += g_trippyLevel * 2.0f;  // 2-6 degrees per frame
        if (g_hueShift >= 360.0f) g_hueShift -= 360.0f;
    } else {
        g_hueShift = 0.0f;
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
        duder.update(chunk_w, chunk_h, ship->pos.x, ship->pos.y, ship->vel.x, ship->vel.y);

    updateParticles();
    for (int i = 0; i < SFX_TRACK_COUNT; i++)
        rebuildSfxLoop(i);

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

    projectiles.emplace_back(noseX, noseY, ship->angle, PHOTON_SPEED);
    Projectile& proj = projectiles.back();

    // Add ship velocity so the photon moves away from the ship at c,
    // regardless of how fast the ship itself is moving.
    proj.vel.x += ship->vel.x;
    proj.vel.y += ship->vel.y;

    proj.initPhysics(physicsWorld);
}

void Game::launchDuder(void) {
    // Find a following duder to launch
    Duder* toLaunch = nullptr;
    for (auto& duder : world_duders) {
        if (duder.is_following && !duder.is_launched && !duder.is_killed) {
            toLaunch = &duder;
            break;
        }
    }
    if (!toLaunch) return;

    toLaunch->is_following = false;
    toLaunch->is_launched = true;
    toLaunch->launch_life = 3.0f;

    // Launch from ship nose in ship's facing direction
    float launchSpeed = 10.0f;
    float noseX = ship->pos.x + cosf(ship->angle) * ship->height2;
    float noseY = ship->pos.y + sinf(ship->angle) * ship->height2;
    toLaunch->pos.x = noseX;
    toLaunch->pos.y = noseY;
    toLaunch->vel.x = cosf(ship->angle) * launchSpeed + ship->vel.x * 0.2f;
    toLaunch->vel.y = sinf(ship->angle) * launchSpeed + ship->vel.y * 0.2f;

    if (b2Body_IsValid(toLaunch->physicsBody)) {
        b2Body_SetTransform(toLaunch->physicsBody,
            {noseX / PIXELS_PER_METER, noseY / PIXELS_PER_METER},
            b2Body_GetRotation(toLaunch->physicsBody));
        physicsWorld.setLinearVelocity(
            toLaunch->physicsBody,
            toLaunch->vel.x * FRAME_RATE,
            toLaunch->vel.y * FRAME_RATE);
    }

    play_croak();
}

void Game::play_impact_sound(Object* obj, GameColor color, SynthRole role) {
    if (music_mode != MUSIC_CUSTOM) return;

    // Get instantaneous (theme/trippy-transformed) color HSV
    GameColor tc = transform_color(color);
    float hue, sat, val;
    rgb_to_hsv(tc.r, tc.g, tc.b, hue, sat, val);
    float obj_size = sqrtf(obj->width2 * obj->width2 + obj->height2 * obj->height2);

    SynthParams params = synth_from_color(hue, sat, val, obj_size, g_colorTheme, role, musicState, (float)g_trippyLevel);
    params.reverb_amount = std::min(1.0f, g_tracerLength / 80.0f);
    auto buf = synth_generate(params);
    int num_samples = (int)buf.size() / 2;

    // Play immediately
    play_wav(buf.data(), num_samples, 44100);

    // Mix/overlay into the per-role loop buffer, quantized to 16th note grid
    int tidx = roleToTrack(role);
    float beat_len = 60.0f / musicState.bpm;
    int loop_samples = (int)(beat_len * 4 * 44100) * 2;  // 4 beats, stereo
    if (loop_samples < 44100) loop_samples = 44100 * 2;
    if ((int)sfx_buffers[tidx].size() != loop_samples) {
        sfx_buffers[tidx].assign(loop_samples, 0);
    }

    // Quantize write position to nearest 16th note
    int sixteenth = loop_samples / 16;
    int quantized_pos = ((int)(musicState.beat_time * 44100 * 2) % loop_samples);
    quantized_pos = (quantized_pos / sixteenth) * sixteenth;

    // Mix new samples into the role's loop buffer
    for (int i = 0; i < (int)buf.size() && i < loop_samples; i++) {
        int idx = (quantized_pos + i) % loop_samples;
        int32_t mixed = (int32_t)sfx_buffers[tidx][idx] + (int32_t)buf[i];
        sfx_buffers[tidx][idx] = (int16_t)std::clamp(mixed, (int32_t)-32000, (int32_t)32000);
    }
    sfx_dirty[tidx] = true;
}

void Game::rebuildSfxLoop(int track_idx) {
    if (!sfx_dirty[track_idx] || !sfx_tracks[track_idx] || sfx_buffers[track_idx].empty()) return;
    sfx_dirty[track_idx] = false;

    MIX_StopTrack(sfx_tracks[track_idx], 0);
    if (sfx_audios[track_idx]) { MIX_DestroyAudio(sfx_audios[track_idx]); sfx_audios[track_idx] = nullptr; }

    int num_samples = (int)sfx_buffers[track_idx].size() / 2;
    int wav_size = 0;
    uint8_t* wav = synth_build_wav(sfx_buffers[track_idx].data(), num_samples, 44100, &wav_size);
    if (!wav) return;

    SDL_IOStream* io = SDL_IOFromMem(wav, wav_size);
    if (io) {
        sfx_audios[track_idx] = MIX_LoadAudio_IO(mixer, io, true, true);
        if (sfx_audios[track_idx]) {
            MIX_SetTrackAudio(sfx_tracks[track_idx], sfx_audios[track_idx]);
            SDL_PropertiesID props = SDL_CreateProperties();
            SDL_SetNumberProperty(props, MIX_PROP_PLAY_LOOPS_NUMBER, -1);
            MIX_PlayTrack(sfx_tracks[track_idx], props);
            SDL_DestroyProperties(props);
        }
    }
    SDL_free(wav);
}

int Game::roleToTrack(SynthRole role) {
    switch (role) {
        case SynthRole::BASS:    return 0;
        case SynthRole::MID:     return 1;
        case SynthRole::HIHAT:   return 2;
        case SynthRole::GENERAL: return 3;
    }
    return 3;
}

void Game::clearAllSfxLoops(void) {
    for (int i = 0; i < SFX_TRACK_COUNT; i++) {
        sfx_buffers[i].clear();
        sfx_dirty[i] = false;
        if (sfx_tracks[i]) MIX_StopTrack(sfx_tracks[i], 0);
        if (sfx_audios[i]) { MIX_DestroyAudio(sfx_audios[i]); sfx_audios[i] = nullptr; }
    }
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
            cuzer.update(ship->pos.x, ship->pos.y);
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

        if (shipObj && duderObj && !duderObj->is_killed && !duderObj->is_following && !duderObj->is_launched) {
            auto it = std::next(biases.biases.begin(), duderObj->random_val % biases.biases.size());
            duderObj->bias = &(*it);
            duderObj->encounter_pos = duderObj->pos;
            duderObj->is_following = true;
            biases_groked.insert(duderObj->bias->first);
            play_croak();
        }

        // Launched duder collision: play impact sound based on hit object
        if (duderObj && duderObj->is_launched) {
            Object* hitObj = nullptr;
            if (objA != (Object*)duderObj) hitObj = objA;
            else hitObj = objB;

            // Don't trigger on ship collision
            if (!dynamic_cast<Ship*>(hitObj) && hitObj) {
                // Get the hit object's color
                GameColor hitColor = map_rgb(255, 255, 255);
                if (auto* a = dynamic_cast<Asteroid*>(hitObj)) hitColor = a->getColor();
                else if (auto* c = dynamic_cast<Cuzer*>(hitObj)) hitColor = c->getColor();
                else if (auto* b = dynamic_cast<Body*>(hitObj)) hitColor = b->color;
                else if (auto* d = dynamic_cast<Duder*>(hitObj)) hitColor = d->color;

                play_impact_sound(hitObj, hitColor);
                spawnExplosion(duderObj->pos.x, duderObj->pos.y,
                               duderObj->width2, duderObj->color, 10);
            }
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
                play_impact_sound(asteroidObj, asteroidObj->getColor(), SynthRole::HIHAT);
                spawnExplosion(asteroidObj->pos.x, asteroidObj->pos.y,
                               asteroidObj->getRadius(), asteroidObj->getColor(),
                               15 + (int)(asteroidObj->getRadius() * 0.5f));
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
            float damage = 100.0f;
            switch (asteroidObj->getSize()) {
                case AsteroidSize::SMALL: damage = 50.0f; break;
                case AsteroidSize::MEDIUM: damage = 150.0f; break;
                case AsteroidSize::LARGE: damage = 300.0f; break;
            }
            ship->fuel -= damage;
            if (ship->fuel < 0) ship->fuel = 0;
            play_impact_sound(asteroidObj, asteroidObj->getColor(), SynthRole::MID);
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
                play_impact_sound(cuzerObj, cuzerObj->getColor(), SynthRole::HIHAT);
                // takeHit() returns true if cuzer is destroyed
                if (cuzerObj->takeHit()) {
                    spawnExplosion(cuzerObj->pos.x, cuzerObj->pos.y,
                                   cuzerObj->getRadius(), cuzerObj->getColor(),
                                   20 + (int)(cuzerObj->getRadius() * 0.8f));
                    cuzersToDestroy.push_back(cuzerObj);

                    // Clear all SFX loops when a cuzer is killed by bullets
                    clearAllSfxLoops();
                }
            }
            if (!projectileMarked) {
                projectileObj->expired = true;
                projectilesToDestroy.push_back(projectileObj);
            }
        }

        // Ship-cuzer collision: damage ship (lose fuel)
        if (shipObj && cuzerObj && !cuzerObj->isDestroyed()) {
            float damage = 100.0f;
            switch (cuzerObj->getSize()) {
                case CuzerSize::SMALL: damage = 40.0f; break;
                case CuzerSize::MEDIUM: damage = 100.0f; break;
                case CuzerSize::LARGE: damage = 200.0f; break;
            }
            ship->fuel -= damage;
            if (ship->fuel < 0) ship->fuel = 0;
            play_impact_sound(cuzerObj, cuzerObj->getColor(), SynthRole::MID);
        }

        // Ship-body collision: play bass sound
        if (shipObj && bodyObj) {
            play_impact_sound(bodyObj, bodyObj->color, SynthRole::MID);
        }

        // Duder-body collisions handled by Box2D physics
    }

    // Hit events: generate sounds based on what's colliding
    for (int i = 0; i < contactEvents.hitCount; i++) {
        b2ContactHitEvent* hitEvent = contactEvents.hitEvents + i;
        if (!b2Shape_IsValid(hitEvent->shapeIdA) || !b2Shape_IsValid(hitEvent->shapeIdB))
            continue;

        Object* objA = (Object*)b2Shape_GetUserData(hitEvent->shapeIdA);
        Object* objB = (Object*)b2Shape_GetUserData(hitEvent->shapeIdB);
        if (!objA || !objB) continue;

        auto getObjColor = [](Object* obj) -> GameColor {
            if (auto* a = dynamic_cast<Asteroid*>(obj)) return a->getColor();
            if (auto* c = dynamic_cast<Cuzer*>(obj)) return c->getColor();
            if (auto* b = dynamic_cast<Body*>(obj)) return b->color;
            if (auto* d = dynamic_cast<Duder*>(obj)) return d->color;
            return map_rgb(255, 255, 255);
        };

        Duder* duderObj = dynamic_cast<Duder*>(objA);
        if (!duderObj) duderObj = dynamic_cast<Duder*>(objB);

        Projectile* projObj = dynamic_cast<Projectile*>(objA);
        if (!projObj) projObj = dynamic_cast<Projectile*>(objB);

        Ship* shipObj = dynamic_cast<Ship*>(objA);
        if (!shipObj) shipObj = dynamic_cast<Ship*>(objB);

        // Ship collisions → BASS
        if (shipObj) {
            Object* hitObj = (objA != (Object*)shipObj) ? objA : objB;
            play_impact_sound(hitObj, getObjColor(hitObj), SynthRole::MID);
        }

        // Launched duder collisions → BASS
        if (duderObj && duderObj->is_launched) {
            Object* hitObj = (objA != (Object*)duderObj) ? objA : objB;
            if (!dynamic_cast<Ship*>(hitObj)) {
                GameColor hitColor = getObjColor(hitObj);
                play_impact_sound(hitObj, hitColor, SynthRole::BASS);
                spawnExplosion(hitEvent->point.x * PIXELS_PER_METER,
                               hitEvent->point.y * PIXELS_PER_METER,
                               5.0f + hitEvent->approachSpeed * 2.0f,
                               hitColor, 5);
            }
        }

        // Bullet/projectile collisions → HIHAT
        if (projObj) {
            Object* hitObj = (objA != (Object*)projObj) ? objA : objB;
            if (!dynamic_cast<Ship*>(hitObj)) {
                play_impact_sound(hitObj, getObjColor(hitObj), SynthRole::HIHAT);
            }
        }
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
    if (!duder->is_killed && !duder->is_following) return;

    // Use encounter position (world coords) converted to screen coords
    float anchor_x = duder->is_following ? duder->encounter_pos.x : duder->pos.x;
    float anchor_y = duder->is_following ? duder->encounter_pos.y : duder->pos.y;
    float sx = cam_wx(anchor_x);
    float sy = cam_wy(anchor_y);

    // Skip if off-screen
    if (sx < -200 || sx > window_width + 200 || sy < -200 || sy > window_height + 200)
        return;

    GameColor dc = duder->color;
    SDL_Color title_color = {255, 255, 255, 255};
    SDL_Color body_color = {
        (Uint8)(std::min(1.0f, dc.r * 1.3f) * 255),
        (Uint8)(std::min(1.0f, dc.g * 1.3f) * 255),
        (Uint8)(std::min(1.0f, dc.b * 1.3f) * 255), 255
    };

    // Split description into lines
    string txt = duder->bias->second;
    vector<string> lines;
    int maxwords = 6;
    int wordcount = 0;
    size_t line_start = 0;
    for (size_t i = 0; i <= txt.size(); i++) {
        if (i == txt.size() || txt[i] == ' ') {
            wordcount++;
            if (wordcount >= maxwords || i == txt.size()) {
                lines.push_back(txt.substr(line_start, i - line_start));
                line_start = i + 1;
                wordcount = 0;
            }
        }
    }

    // Measure widths to find the box size
    int title_w = 0, title_h = 0;
    TTF_GetStringSize(font, duder->bias->first.c_str(), 0, &title_w, &title_h);
    float scaled_title_w = title_w * 0.9f;

    float max_line_w = scaled_title_w;
    for (auto& line : lines) {
        int lw = 0, lh = 0;
        TTF_GetStringSize(font, line.c_str(), 0, &lw, &lh);
        if (lw > max_line_w) max_line_w = lw;
    }

    float padding = 12.0f;
    float title_line_h = 22.0f;
    float sep_h = 8.0f;
    float body_line_h = 18.0f;
    float box_w = max_line_w + padding * 2;
    float box_h = padding + title_line_h + sep_h + lines.size() * body_line_h + padding;
    float box_x = sx - box_w / 2;
    float box_y = sy - 50 - padding;

    // Draw box background (screen-space — coords already camera-transformed)
    float save_zoom = g_camera_zoom;
    float save_cx = g_camera_x, save_cy = g_camera_y;
    g_camera_zoom = 1.0f;
    g_camera_x = g_screen_cx;
    g_camera_y = g_screen_cy;

    GameColor bg = {0.0f, 0.0f, 0.0f, 0.7f};
    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);
    draw_filled_rounded_rect(box_x, box_y, box_x + box_w, box_y + box_h,
                             6, 6, bg);

    // Draw box border in duder color
    GameColor border = {dc.r, dc.g, dc.b, 0.6f};
    draw_rounded_rect(box_x, box_y, box_x + box_w, box_y + box_h,
                      6, 6, border, 1.5f);

    // Draw title
    float line_y = sy - 50;
    draw_text_scaled(duder->bias->first.c_str(), sx, line_y, title_color, 0.9f, true);
    line_y += title_line_h;

    // Separator line
    float sep_w = max_line_w * 0.6f;
    GameColor sep_color = {dc.r, dc.g, dc.b, 0.4f};
    draw_line(sx - sep_w/2, line_y, sx + sep_w/2, line_y, sep_color, 1.0f);
    line_y += sep_h;

    // Draw body lines
    for (auto& line : lines) {
        draw_text(line.c_str(), sx, line_y, body_color, true);
        line_y += body_line_h;
    }

    g_camera_zoom = save_zoom;
    g_camera_x = save_cx;
    g_camera_y = save_cy;
}

void Game::spawnExplosion(float x, float y, float radius, GameColor color, int count) {
    for (int i = 0; i < count; i++) {
        float angle = (rand() % 3600) / 3600.0f * 2.0f * M_PI;
        float speed = 1.0f + (rand() % 100) / 20.0f;
        float life = 0.3f + (rand() % 100) / 100.0f * 0.7f;  // 0.3-1.0s
        float sz = 2.0f + (rand() % (int)(radius * 0.3f + 1));

        // Vary the color slightly per particle
        float cr = std::min(1.0f, color.r + ((rand() % 60) - 30) / 255.0f);
        float cg = std::min(1.0f, color.g + ((rand() % 60) - 30) / 255.0f);
        float cb = std::min(1.0f, color.b + ((rand() % 60) - 30) / 255.0f);

        particles.push_back({
            x, y,
            cosf(angle) * speed, sinf(angle) * speed,
            life, life,
            sz,
            {cr, cg, cb, 1.0f}
        });
    }
}

void Game::updateParticles(void) {
    for (auto it = particles.begin(); it != particles.end(); ) {
        it->life -= 1.0f / 60.0f;
        if (it->life <= 0.0f) {
            it = particles.erase(it);
        } else {
            it->x += it->vx;
            it->y += it->vy;
            it->vx *= 0.97f;
            it->vy *= 0.97f;
            ++it;
        }
    }
}

void Game::drawParticles(void) {
    for (auto& p : particles) {
        float alpha = p.life / p.max_life;
        float sz = p.size * alpha;
        GameColor c = {p.color.r, p.color.g, p.color.b, alpha};
        draw_filled_ellipse(p.x, p.y, sz, sz, c);
    }
}

void Game::play_wav(int16_t* samples, int num_samples, int sample_rate) {
    int wav_size = 0;
    uint8_t* wav = synth_build_wav(samples, num_samples, sample_rate, &wav_size);
    if (!wav) return;

    SDL_IOStream* io = SDL_IOFromMem(wav, wav_size);
    if (io) {
        MIX_Audio* audio = MIX_LoadAudio_IO(mixer, io, true, true);
        if (audio) {
            MIX_PlayAudio(mixer, audio);
            MIX_DestroyAudio(audio);
        }
    }
    SDL_free(wav);
}

void Game::play_croak(void){
    if (music_mode == MUSIC_OFF) return;
    auto buf = synth_croak(g_colorTheme);
    int num_samples = (int)buf.size() / 2;
    play_wav(buf.data(), num_samples, 44100);
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
            // Activate tracers
            tracerLengthMax = 10 + (int)(loot->value * 15);
            g_tracerLength = tracerLengthMax;
            tracerTimerMax = 5.0f + loot->value;
            tracerTimer = tracerTimerMax;
            break;
        case MUSHROOM:
            // Activate trippiness
            g_trippyLevel = (int)loot->value;
            g_colorTheme = static_cast<int>(ColorTheme::NEON);
            trippyTimer = 5.0f + loot->value * 2.0f;
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

    // Zoom out at higher speeds so the player can see more
    float speed = sqrtf(ship->vel.x * ship->vel.x + ship->vel.y * ship->vel.y);
    float zoom_min = 0.5f;
    float speed_for_min_zoom = 300.0f;
    float t = std::min(speed / speed_for_min_zoom, 1.0f);
    camera_target_zoom = lerp(1.0f, zoom_min, t);

    // Smooth lerp toward targets
    camera_x = lerp(camera_x, camera_target_x, lerp_speed);
    camera_y = lerp(camera_y, camera_target_y, lerp_speed);
    camera_zoom = lerp(camera_zoom, camera_target_zoom, 0.01f);

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
    if (touchInput.state.joystick_x != 0)
        ship->rotate(touchInput.state.joystick_x);
    if (touchInput.state.thrust_pressed)
        ship->thrust(1);
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
    float save_zoom = g_camera_zoom;
    float save_cx_cam = g_camera_x, save_cy_cam = g_camera_y;
    g_camera_zoom = 1.0f;
    g_camera_x = g_screen_cx;
    g_camera_y = g_screen_cy;

    float cx = window_width / 2.0f;
    float cy = window_height / 2.0f;
    float s = window_height / 1440.0f;  // scale factor

    SDL_Color white = {255, 255, 255, 255};
    SDL_Color gray = {128, 128, 128, 255};
    SDL_Color highlight = {100, 200, 255, 255};

    GameColor border_normal = map_rgb(100, 100, 100);
    GameColor border_selected = map_rgb(100, 200, 255);

    // Title
    draw_text_scaled("S H I P P Y", cx, cy - 280 * s, white, 5.0f * s, true);

    // Difficulty row
    float row_y = cy - 80 * s;
    {
        char buf[64];
        snprintf(buf, sizeof(buf), "<  %d  >", difficulty);
        SDL_Color col = (menu_selection == 0) ? highlight : white;
        draw_text_scaled("Difficulty:", cx - 240 * s, row_y + 6 * s, col, 1.8f * s, false);
        draw_text_scaled(buf, cx + 90 * s, row_y, col, 2.0f * s, false);

        menu_rects[0] = {cx - 240 * s, row_y, 500 * s, 54 * s};
    }

    // Music row
    row_y = cy + 20 * s;
    {
        SDL_Color col = (menu_selection == 1) ? highlight : white;
        draw_text_scaled("Music:", cx - 240 * s, row_y + 6 * s, col, 1.8f * s, false);

        const char* labels[] = {"OFF", "CUSTOM", "ON"};
        float opt_x[] = {cx - 60 * s, cx + 80 * s, cx + 280 * s};
        for (int m = 0; m < 3; m++) {
            bool active = ((int)music_mode == m);
            char lbl[16];
            snprintf(lbl, sizeof(lbl), active ? "[%s]" : " %s ", labels[m]);
            SDL_Color mc = active ? (menu_selection == 1 ? highlight : white) : gray;
            draw_text_scaled(lbl, opt_x[m], row_y + 6 * s, mc, 1.6f * s, false);
        }

        menu_rects[1] = {cx - 240 * s, row_y, 600 * s, 54 * s};
    }

    // Start button
    row_y = cy + 130 * s;
    {
        float btn_w = 300 * s, btn_h = 70 * s;
        float btn_x = cx - btn_w / 2;
        float btn_y = row_y;
        GameColor border = (menu_selection == 2) ? border_selected : border_normal;
        draw_rounded_rect(btn_x, btn_y, btn_x + btn_w, btn_y + btn_h, 12 * s, 12 * s, border, 3.0f);

        SDL_Color col = (menu_selection == 2) ? highlight : white;
        draw_text_scaled("S T A R T", cx, btn_y + 12 * s, col, 2.0f * s, true);

        menu_rects[2] = {btn_x, btn_y, btn_w, btn_h};
    }

    // Nav hint
    draw_text_scaled("arrows / tap to navigate    enter to select", cx, cy + 260 * s, gray, 1.2f * s, true);

    g_camera_zoom = save_zoom;
    g_camera_x = save_cx_cam;
    g_camera_y = save_cy_cam;
}

void Game::draw_pause(void) {
    // Dim overlay
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 160);
    SDL_FRect overlay = {0, 0, (float)window_width, (float)window_height};
    SDL_RenderFillRect(renderer, &overlay);

    float save_zoom = g_camera_zoom;
    float save_cx_cam = g_camera_x, save_cy_cam = g_camera_y;
    g_camera_zoom = 1.0f;
    g_camera_x = g_screen_cx;
    g_camera_y = g_screen_cy;

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

    g_camera_zoom = save_zoom;
    g_camera_x = save_cx_cam;
    g_camera_y = save_cy_cam;
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
                if (menu_selection == 1) music_mode = (MusicMode)((music_mode + 1) % 3);
                break;
            case SDL_SCANCODE_RIGHT:
            case SDL_SCANCODE_D:
                if (menu_selection == 0 && difficulty < 10) difficulty++;
                if (menu_selection == 1) music_mode = (MusicMode)((music_mode + 1) % 3);
                break;
            case SDL_SCANCODE_RETURN:
            case SDL_SCANCODE_SPACE:
                if (menu_selection == 1) {
                    music_mode = (MusicMode)((music_mode + 1) % 3);
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
                    music_mode = (MusicMode)((music_mode + 1) % 3);
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
            // Double-tap fire detection (keyboard)
            if (event.type == SDL_EVENT_KEY_DOWN &&
                event.key.scancode == SDL_SCANCODE_SPACE &&
                !event.key.repeat) {
                Uint64 now = SDL_GetTicks();
                if (now - lastFireTapTime < 400) {
                    launchDuder();
                    lastFireTapTime = 0;
                } else {
                    lastFireTapTime = now;
                }
            }
            // Double-tap fire detection (touch)
            if (event.type == SDL_EVENT_FINGER_DOWN) {
                float px = event.tfinger.x * window_width;
                float py = event.tfinger.y * window_height;
                if (touchInput.point_in_zone(px, py, touchInput.fire_zone)) {
                    Uint64 now = SDL_GetTicks();
                    if (now - lastFireTapTime < 400) {
                        launchDuder();
                        lastFireTapTime = 0;
                    } else {
                        lastFireTapTime = now;
                    }
                }
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

            // Draw HUD on top of buffer (after trail copy so it doesn't persist in trails)
            SDL_SetRenderTarget(renderer, buffer);
            draw_hud();

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

    for (int i = 0; i < SFX_TRACK_COUNT; i++) {
        if (sfx_tracks[i]) MIX_DestroyTrack(sfx_tracks[i]);
        if (sfx_audios[i]) MIX_DestroyAudio(sfx_audios[i]);
    }

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
