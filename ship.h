
#ifndef _SHIP_H_
#define _SHIP_H_

#include "geom.h"
#include "space.h"
#include "object.h"

class PhysicsWorld;

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

    void gravitate_bodies(Space &space);
    void update(void);
    void rotate(float scale);
    void thrust(float scale);
    void brake(float scale);
    void draw(void);

    void initPhysics(PhysicsWorld& world) override;
    void syncFromPhysics() override;
    void setPhysicsWorld(PhysicsWorld* world) { physicsWorldPtr = world; }

    Point vel;
    Point accel;
    float fuel, fuel_start;
    float angle;           // Ship facing angle in radians (0 = right, PI/2 = down)
    float angularVelocity; // Angular velocity for smooth rotation

    private:
    Point prev_pos;
    float offset;
    float prev_t, cur_t;
    float mass;
    float thrustPower;
    float rotatePower;
    bool isThrusting;
    bool isBraking;
    int thrustFlameCounter;
    int brakeFlameCounter;
    float thick;
    PhysicsWorld* physicsWorldPtr = nullptr;

    // Ship geometry (computed once, used for drawing and physics)
    float body_length;
    float body_width;
    float dome_radius;
    float engine_radius;
    float engine_spread;

    void draw_flame(float tx, float ty, float scale, float flameAngle);
    void draw_thrust_flame(void);
    void draw_brake_flame(void);


};




#endif

