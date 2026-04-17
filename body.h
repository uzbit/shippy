#ifndef _BODY_H_
#define _BODY_H_

#include "sdl_compat.h"
#include "object.h"

class PhysicsWorld;

class Body : public Object{

    public:
    Body(){}
    Body(float x, float y, float width, float height, GameColor color);
    ~Body();

    void draw(void);
    void initPhysics(PhysicsWorld& world) override;

    int round;
    GameColor color;
    float density;
    float gravityStrength;  // 0 = normal body, >0 = gravity well (attracts other objects)
    bool isGravityWell;
};



#endif
