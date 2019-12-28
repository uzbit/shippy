#ifndef _LOOT_H_
#define _LOOT_H_

#include <allegro5/allegro5.h>
#include "object.h"
#include "collision.h"

enum LootType{
    FUEL,
    BOOST,
    NUM_LOOT
};

class Loot : public Object{

    public:
    Loot(){}
    Loot(float x, float y, float width, float height, ALLEGRO_COLOR color, LootType type);
    ~Loot(){}
    
    void draw(void);

    float value;
    ALLEGRO_COLOR color;
    LootType type;
    Collision prev_collision;
};
    


#endif
