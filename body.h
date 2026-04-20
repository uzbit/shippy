#ifndef _BODY_H_
#define _BODY_H_

#include <vector>
#include "sdl_compat.h"
#include "object.h"

class PhysicsWorld;

struct Crater {
    float angle;    // position on surface (radians)
    float dist;     // distance from center (fraction of radius)
    float radius;   // crater size
};

class Body : public Object{

    public:
    Body(){}
    Body(float x, float y, float width, float height, GameColor color);
    ~Body();

    void draw(void);
    void initPhysics(PhysicsWorld& world) override;

    GameColor color;
    float density;
    float gravityStrength;
    bool isGravityWell;

    private:
    static const int NUM_EDGE_VERTS = 32;
    float edgeRadii[NUM_EDGE_VERTS]; // jagged edge: radius multiplier per vertex
    float rotation;                   // random rotation angle
    std::vector<Crater> craters;
};



#endif
