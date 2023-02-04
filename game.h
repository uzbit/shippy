#ifndef _GAME_H_
#define _GAME_H_

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>
#include <vector>
#include <set>
#include "ship.h"
#include "loot.h"
#include "space.h"
#include "biases.h"
#include "duder.h"
#include "starfield.h"

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
    void update_game(ALLEGRO_EVENT &e);
    void update_space(void);
    int get_space_index(void);
    void draw_info(void);
    void collide_ship_bodies(void);
    void collide_ship_loot(void);
    void collide_ship_duder(void);
    void collide_duder_bodies(void);
    void draw_duder_bias(Duder *duder);
    void apply_loot(Loot *loot);

    bool done;
    ALLEGRO_EVENT_QUEUE* event_queue;
    ALLEGRO_TIMER* timer;
    ALLEGRO_DISPLAY* display;
    ALLEGRO_FONT *font;
    ALLEGRO_VOICE *voice;
    ALLEGRO_MIXER *mixer;
    ALLEGRO_AUDIO_STREAM *stream;

    Ship *ship; // make multiplayer 
    vector<Space> spaces;
    int coordx, coordy;
    int space_index;
    Biases biases;
    set<string> biases_groked;
    Starfield starfield;
    int window_width, window_height;
    
};


#endif