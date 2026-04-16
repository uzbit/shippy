
#include <iostream>

#include "sdl_compat.h"
#include "duder.h"
#include "geom.h"
#include "defines.h"
#include "physics_world.h"

using namespace std;

Duder::Duder(float x, float y, float width, float height)
:Object(x, y, width, height){
    pos.x = x;
    pos.y = y;
    vel.x = ((rand() % 300) - 150)/50.0;
    vel.y = ((rand() % 300) - 150)/50.0;
    color = map_rgb(rand()%255, rand()%255, rand()%255);
    random_val = rand();
    thick = rand() % 10  +1;
    is_killed = false;
}

void Duder::initPhysics(PhysicsWorld& world) {
    physicsWorldPtr = &world;
    // Use DYNAMIC instead of KINEMATIC so duders collide with static bodies
    physicsBody = world.createBody(this, PhysicsBodyType::DYNAMIC, 0.5f, 0.0f, 1.0f);
    if (b2Body_IsValid(physicsBody)) {
        // Disable gravity for duders so they float
        b2Body_SetGravityScale(physicsBody, 0.0f);
        b2Body_SetFixedRotation(physicsBody, true);
        b2Body_SetLinearDamping(physicsBody, 0.0f);
        // vel is pixels/frame, convert to pixels/second for physics
        world.setLinearVelocity(physicsBody, vel.x * FRAME_RATE, vel.y * FRAME_RATE);
    }
}

void Duder::syncFromPhysics() {
    if (!physicsWorldPtr || !b2Body_IsValid(physicsBody)) return;

    b2Vec2 physPos = physicsWorldPtr->getPosition(physicsBody);
    b2Vec2 physVel = physicsWorldPtr->getLinearVelocity(physicsBody);
    pos.x = physPos.x;
    pos.y = physPos.y;
    // Sync velocity back (convert from pixels/second to pixels/frame)
    vel.x = physVel.x / FRAME_RATE;
    vel.y = physVel.y / FRAME_RATE;
}

void Duder::update(int w, int h){
    if (!is_killed){
        if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
            syncFromPhysics();
        } else {
            pos.x += vel.x;
            pos.y += vel.y;
        }
    }
}


void Duder::draw(void){
    computeRect();

    if (!is_killed){
        draw_ellipse(
            pos.x, pos.y-height2, width2, height2,
            color, thick
        );
        draw_filled_ellipse(
            pos.x, pos.y, width, height2,
            color
        );
    }
}
