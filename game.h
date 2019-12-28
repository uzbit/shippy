#ifndef _GAME_H_
#define _GAME_H_

#include <allegro5/allegro5.h>
#include <vector>
#include "ship.h"
#include "space.h"

using namespace std;


class Game{

    public:
    Game();
    ~Game();

    void init_graphics(void);
    void init_game(void);
    void add_space(int coordx, int coordy);
    void update_graphics(void);
    void update_game(void);
    void update_space(void);
    int get_space_index(void);
    void collide_objects(void);
    void loop(void);
    void abort(const char* message);
    void shutdown(void);

    private:
    bool done;
    ALLEGRO_EVENT_QUEUE* event_queue;
    ALLEGRO_TIMER* timer;
    ALLEGRO_DISPLAY* display;

    Ship *ship;
    vector<Space> spaces;
    int coordx, coordy;
    int space_index;
};


#endif