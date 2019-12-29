#ifndef _GAME_H_
#define _GAME_H_

#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <vector>
#include "ship.h"
#include "loot.h"
#include "space.h"

using namespace std;


class Game{

    public:
    Game();
    ~Game();

    void init_graphics(void);
    void init_game(void);
    
    void add_space(int coordx, int coordy);
    void adjust_ship_position(void);
    void update_graphics(void);
    void update_game(void);
    void update_space(void);
    int get_space_index(void);
    void draw_info(void);
    void collide_ship_bodies(void);
    void collide_ship_loot(void);
    void apply_loot(Loot *loot);

    void loop(void);
    void abort(const char* message);
    void shutdown(void);

    private:
    bool done;
    ALLEGRO_EVENT_QUEUE* event_queue;
    ALLEGRO_TIMER* timer;
    ALLEGRO_DISPLAY* display;
    ALLEGRO_FONT *font;

    Ship *ship;
    vector<Space> spaces;
    int coordx, coordy;
    int space_index;
};


#endif