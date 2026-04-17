#ifndef _OBJECT_H_
#define _OBJECT_H_

#include <map>
#include <box2d/box2d.h>
#include "geom.h"

class PhysicsWorld;

class Object{
    public:
    Object() : physicsBody(b2_nullBodyId) {}
    Object(float x, float y, float width, float height);
    virtual ~Object(){}

    virtual void initPhysics(PhysicsWorld& world) {}
    virtual void syncFromPhysics() {}
    virtual void destroyPhysics(PhysicsWorld& world);

    Point pos;
    float width, height;
    float width2, height2;
    int chunk_cx, chunk_cy;  // Which chunk this entity belongs to
    b2BodyId physicsBody;
};


#endif