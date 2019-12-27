
#ifndef _SHIP_H_
#define _SHIP_H_

#include "geom.h"

class Ship{

    public:
    Ship();
    Ship(int x, int y, float fuel, float mass);
    ~Ship();


    void draw();

    private:
    Point center;
    float fuel;
    float mass;
    
};




#endif

