
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
    void thrust_vertical(float scale);
    void thrust_horizontal(float scale);
    void draw(void);

    void initPhysics(PhysicsWorld& world) override;
    void syncFromPhysics() override;
    void setPhysicsWorld(PhysicsWorld* world) { physicsWorldPtr = world; }

    Point vel;
    Point accel;
    float fuel, fuel_start;

    private:
    Point prev_pos;
    float offset;
    float prev_t, cur_t;
    float mass;
    float thrustx, thrusty;
    int thrust_dir;
    int flame_counter[4];
    float thick;
    PhysicsWorld* physicsWorldPtr = nullptr;

    void draw_flame(float tx, float ty, float scale, float angle);
    void draw_flames(void);


};




#endif

