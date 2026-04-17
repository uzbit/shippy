
#ifndef _DUDER_H_
#define _DUDER_H_

#include <string>
#include <utility>
#include "sdl_compat.h"
#include "geom.h"
#include "object.h"

using namespace std;

class PhysicsWorld;

class Duder : public Object{

    public:
    Duder(){};
    Duder(float x, float y, float width, float height);
    ~Duder(){};

    void update(int w, int h, float target_x = 0, float target_y = 0, float target_vx = 0, float target_vy = 0);
    void draw(void);
    void initPhysics(PhysicsWorld& world) override;
    void syncFromPhysics() override;
    void setPhysicsWorld(PhysicsWorld* world) { physicsWorldPtr = world; }

    Point vel;
    Point encounter_pos;  // where the duder was first touched
    float angle;           // rotation in radians
    bool is_killed;
    bool is_following;
    bool is_launched;
    float launch_life;     // seconds remaining before launched duder expires
    int random_val;
    pair<const string, string> *bias;
    GameColor color;

    private:
    float thick;
    PhysicsWorld* physicsWorldPtr = nullptr;

};




#endif

