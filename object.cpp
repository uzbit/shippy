
#include "object.h"
#include "physics_world.h"

Object::Object(float x, float y, float width, float height)
:width(width), height(height), chunk_cx(0), chunk_cy(0), physicsBody(b2_nullBodyId){
    pos.x = x;
    pos.y = y;
    width2 = width/2;
    height2 = height/2;
}

void Object::destroyPhysics(PhysicsWorld& world) {
    if (b2Body_IsValid(physicsBody)) {
        world.destroyBody(physicsBody);
        physicsBody = b2_nullBodyId;
    }
}
