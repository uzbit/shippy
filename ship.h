
#ifndef _SHIP_H_
#define _SHIP_H_

#include <allegro5/allegro5.h>
#include "geom.h"
#include "object.h"


enum ThrustDirection{
    NONE = 0,
    LEFT = 1, 
    RIGHT = 2, 
    UP = 4, 
    DOWN = 8
};

class Ship : public Object{

    public:
    Ship(){};
    Ship(float x, float y, float fuel, float mass);
    ~Ship(){};

    void update(ALLEGRO_EVENT &e);
    void thrust_vertical(float scale);
    void thrust_horizontal(float scale);
    void draw(void);

    Point vel;
    Point accel;
    float fuel, fuel_start;
    
    private:
    Point prev_pos;
    float offset;
    float prev_t, cur_t;
    float mass;
    float thrustx, thrusty;
    int thrust_dir;
    int flame_counter[4];
    float thick;
    
    void draw_flame(ALLEGRO_TRANSFORM *transform);
    void draw_flames(void);
    

};




#endif

