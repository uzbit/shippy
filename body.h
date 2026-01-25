#ifndef _BODY_H_
#define _BODY_H_

#include "sdl_compat.h"
#include "object.h"
#include "collision.h"

class PhysicsWorld;

class Body : public Object{

    public:
    Body(){}
    Body(float x, float y, float width, float height, GameColor color, bool filled);
    ~Body(){}

    void draw(void);
    void initPhysics(PhysicsWorld& world) override;

    bool filled;
    int round, thick;
    GameColor color;
    float density;
};



#endif
