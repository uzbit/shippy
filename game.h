#ifndef _GAME_H_
#define _GAME_H_

#include <allegro5/allegro5.h>

class Game{

    public:
    Game(){};
    ~Game();


    void abort(const char* message);
    void init_graphics(void);
    void init_game(void);
    void update_graphics(void);
    void loop(void);
    void shutdown(void);

    private:
    bool done;
    ALLEGRO_EVENT_QUEUE* event_queue;
    ALLEGRO_TIMER* timer;
    ALLEGRO_DISPLAY* display;

};


#endif