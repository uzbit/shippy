#ifndef _LOOT_H_
#define _LOOT_H_

#include "sdl_compat.h"
#include "object.h"
#include "collision.h"

class PhysicsWorld;

enum LootType{
    FUEL,
    BOOST,
    MUSHROOM,
    NUM_LOOT
};

class Loot : public Object{

    public:
    Loot(){}
    Loot(float x, float y, float width, float height, GameColor color, LootType type);
    ~Loot(){}

    void draw(void);
    void initPhysics(PhysicsWorld& world) override;

    float value;
    GameColor color;
    LootType type;
};



#endif
