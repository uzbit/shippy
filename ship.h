
#ifndef _SHIP_H_
#define _SHIP_H_

#include "geom.h"
#include "object.h"

class Ship : public Object{

    public:
    Ship();
    Ship(float x, float y, float fuel, float mass);
    ~Ship();

    void update();
    void thrust_vertical(float scale);
    void thrust_horizontal(float scale);
    void draw();

    Point vel;
    Point accel;
    
    float mass;
    float fuel, fuel_start;
    float thrustx, thrusty;
};




#endif

