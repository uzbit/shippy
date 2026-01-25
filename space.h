#ifndef _SPACE_H_
#define _SPACE_H_

#include <vector>
#include <list>

#include "body.h"
#include "loot.h"
#include "duder.h"
#include "geom.h"

using namespace std;

class Space {
    public:
    Space(){};
    Space(int body_count, int coordx, int coordy, int window_w, int window_h, bool gravitate);
    ~Space(){};

    void init(int difficulty);
    void draw(void);
    Point getStartPos(float width, float height);
    
    Body **bodies; //should just use a vector, but we talkin bout practice.
    int body_count;
    int coordx, coordy;
    int window_width, window_height;
    list<Loot> loots;
    list<Duder> duders;
    bool gravitate_bodies;

};

#endif