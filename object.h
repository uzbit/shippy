#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <map>
#include <box2d/box2d.h>
#include "geom.h"
#include "collision.h"

class PhysicsWorld;

class Object{
    public:
    Object() : physicsBody(b2_nullBodyId) {}
    Object(float x, float y, float width, float height);
    virtual ~Object(){}

    Collision collides(Object *obj);

    Rect computeRect(void){
        rect = Rect(
            pos.x-width2, pos.y-height2,
            pos.x+width2, pos.y+height2
        );
        return rect;
    }

    virtual void initPhysics(PhysicsWorld& world) {}
    virtual void syncFromPhysics() {}
    virtual void destroyPhysics(PhysicsWorld& world);

    Point pos;
    Rect rect;
    float width, height;
    float width2, height2;
    map<Object *, Collision> prev_collision_map;
    b2BodyId physicsBody;
};


#endif