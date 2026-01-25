#ifndef _GAME_H_
#define _GAME_H_

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include <vector>
#include <deque>
#include <set>
#include "ship.h"
#include "loot.h"
#include "space.h"
#include "biases.h"
#include "duder.h"
#include "starfield.h"
#include "physics_world.h"

using namespace std;


class Game{

    public:
    Game();
    ~Game();

    void init_graphics(void);
    void init_game(void);
    void loop(void);
    void abort(const char* message);
    void shutdown(void);

    int difficulty;
    bool music_on;
    float fullscreen;

    private:
    void add_space(int coordx, int coordy);
    void adjust_ship_position(void);
    void update_graphics(void);
    void update_game(void);
    void update_space(void);
    int get_space_index(void);
    void draw_info(void);
    void draw_duder_bias(Duder *duder);
    void apply_loot(Loot *loot);
    void handle_input(void);
    void initSpacePhysics(Space& space);
    void enableSpacePhysics(Space& space);
    void disableSpacePhysics(Space& space);
    void processCollisions(void);

    bool done;
    SDL_Window* window;
    SDL_Renderer* renderer;
    TTF_Font* font;
    MIX_Mixer* mixer;
    MIX_Audio* music_audio;
    MIX_Track* music_track;
    SDL_Texture* buffer;

    Ship *ship; // make multiplayer
    deque<Space> spaces;
    int coordx, coordy;
    int space_index;
    int prev_space_index;
    Biases biases;
    set<string> biases_groked;
    Starfield starfield;
    int window_width, window_height;
    bool redraw;
    Uint64 last_frame_time;
    PhysicsWorld physicsWorld;

};


#endif
