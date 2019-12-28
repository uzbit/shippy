#ifndef _SPACE_H_
#define _SPACE_H_

#include <vector>

#include "body.h"
#include "loot.h"

using namespace std;

class Space {
    public:
    Space(){};
    Space(int body_count, int coordx, int coordy);
    ~Space(){};

    void init(void);
    void draw(void);
    
    Body **bodies; //should just use a vector, but we talkin bout practice.
    int body_count;
    int coordx, coordy;

    vector<Loot> loots;

};

#endif