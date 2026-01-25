
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

    void update(int w, int h);
    void draw(void);
    void initPhysics(PhysicsWorld& world) override;
    void syncFromPhysics() override;
    void setPhysicsWorld(PhysicsWorld* world) { physicsWorldPtr = world; }

    Point vel;
    bool is_killed;
    int random_val;
    pair<const string, string> *bias;
    GameColor color;

    private:
    float thick;
    PhysicsWorld* physicsWorldPtr = nullptr;

};




#endif

