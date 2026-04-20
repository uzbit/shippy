#ifndef _GAME_H_
#define _GAME_H_

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>
#include <list>
#include <set>
#include <map>
#include "ship.h"
#include "loot.h"
#include "space.h"
#include "biases.h"
#include "duder.h"
#include "starfield.h"
#include "physics_world.h"
#include "projectile.h"
#include "asteroid.h"
#include "cuzer.h"
#include "touch_input.h"

using namespace std;

enum GameState {
    STATE_MENU,
    STATE_PLAYING,
    STATE_PAUSED
};

// Impact classification — picks which libpd voice fires.
enum class SynthRole {
    BASS,
    HIHAT,
    MID,
    GENERAL,
};

class Game{

    public:
    Game();
    ~Game();

    void init_graphics(void);
    void init_game(void);
    void abort(const char* message);
    void shutdown(void);

    SDL_AppResult handle_event(const SDL_Event& event);
    SDL_AppResult iterate(void);

    enum MusicMode { MUSIC_OFF, MUSIC_CUSTOM, MUSIC_ON };

    int difficulty;
    MusicMode music_mode;
    float fullscreen;
    unsigned int world_seed;

    private:
    // Core update
    void update_graphics(void);
    void draw_hud(void);
    void update_game(void);
    void draw_info(void);
    void handle_input(void);

    // Chunk management
    void update_chunks(void);
    void load_chunk(int cx, int cy);
    void unload_chunk(int cx, int cy);
    SpaceTraits& get_chunk_traits(int cx, int cy);

    // Gameplay
    void draw_duder_bias(Duder *duder);
    void apply_loot(Loot *loot);
    void play_croak(void);
    void play_impact_sound(Object* obj, GameColor color, SynthRole role = SynthRole::GENERAL);
    void launchDuder(void);
    void processCollisions(void);
    void applyGravityWells(void);
    void fireProjectile(void);
    void updateProjectiles(void);
    void updateAsteroids(void);
    void updateCuzers(void);
    void spawnChildAsteroids(Asteroid& parent);

    // Camera
    void update_camera(void);

    // Menu system
    void draw_menu(void);
    void draw_pause(void);
    void handle_menu_event(const SDL_Event& event);
    void handle_pause_event(const SDL_Event& event);
    void draw_text(const char* text, float x, float y, SDL_Color color, bool center = false);
    void draw_text_scaled(const char* text, float x, float y, SDL_Color color, float scale, bool center = false);
    void draw_sound_labels(void);

    struct SoundLabel {
        std::string name;
        Uint64 start_ms;
    };
    std::vector<SoundLabel> recent_sounds;
    void log_sound_trigger(const char* name);

    GameState state;
    int menu_selection;
    int pause_selection;
    SDL_FRect menu_rects[3];
    SDL_FRect pause_rects[3];

    bool done;
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    MIX_Mixer* mixer;
    MIX_Audio* music_audio;
    MIX_Track* music_track;
    SDL_Texture* buffer;
    SDL_Texture* trailBuffer;

    // Particles
    struct Particle {
        float x, y, vx, vy;
        float life, max_life;
        float size;
        GameColor color;
    };
    vector<Particle> particles;
    void spawnExplosion(float x, float y, float radius, GameColor color, int count);
    void updateParticles(void);
    void drawParticles(void);

    // World
    Ship *ship;
    PhysicsWorld physicsWorld;
    map<pair<int,int>, Chunk> loaded_chunks;
    list<Body*> world_bodies;
    list<Asteroid> world_asteroids;
    list<Cuzer> world_cuzers;
    list<Loot> world_loots;
    list<Duder> world_duders;
    list<Projectile> projectiles;
    int chunk_w, chunk_h;  // Chunk dimensions (set to window_width/height)

    Biases biases;
    set<string> biases_groked;
    Starfield starfield;
    int window_width, window_height;
    Uint64 last_frame_time;
    Uint64 lastFireTime;
    Uint64 fireRate;
    Uint64 lastFireTapTime;
    bool fireWasReleased;
    TouchInput touchInput;

    // Camera state
    float camera_x, camera_y;
    float camera_zoom;
    float camera_target_zoom;
    float camera_target_x, camera_target_y;

    // Loot effect timers (seconds remaining)
    float trippyTimer;
    float tracerTimer;
    float tracerTimerMax;   // total duration for proportional decay
    int tracerLengthMax;    // initial tracer length at pickup
};

#endif
