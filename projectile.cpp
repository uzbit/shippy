#include <SDL3/SDL.h>
#include <math.h>

#include "projectile.h"
#include "physics_world.h"
#include "defines.h"

Projectile::Projectile()
    : Object(0, 0, 6, 6), angle(0), lifetime(30.0f), bounceCount(0),
      maxBounces(5), expired(false), coordx(0), coordy(0), speed(0),
      color(map_rgb(255, 255, 0)) {
}

Projectile::Projectile(float x, float y, float angle, float speed)
    : Object(x, y, 6, 6), angle(angle), lifetime(30.0f), bounceCount(0),
      maxBounces(5), expired(false), coordx(0), coordy(0), speed(speed),
      color(map_rgb(255, 255, 0)) {
    vel.x = cos(angle) * speed;
    vel.y = sin(angle) * speed;
}

void Projectile::initPhysics(PhysicsWorld& world) {
    physicsWorldPtr = &world;
    // Create as dynamic body with high restitution for bouncing
    physicsBody = world.createBody(this, PhysicsBodyType::DYNAMIC, 0.1f, 0.0f, 1.0f);
    if (b2Body_IsValid(physicsBody)) {
        b2Body_SetLinearDamping(physicsBody, 0.0f);
        b2Body_SetGravityScale(physicsBody, 0.0f);  // No gravity for projectiles
        b2Body_SetBullet(physicsBody, true);        // Continuous collision detection
        // Set initial velocity
        world.setLinearVelocity(physicsBody, vel.x, vel.y);
    }
}

void Projectile::syncFromPhysics() {
    if (!physicsWorldPtr || !b2Body_IsValid(physicsBody)) return;

    b2Vec2 physPos = physicsWorldPtr->getPosition(physicsBody);
    b2Vec2 physVel = physicsWorldPtr->getLinearVelocity(physicsBody);

    pos.x = physPos.x;
    pos.y = physPos.y;
    vel.x = physVel.x;
    vel.y = physVel.y;

    if (vel.x != 0 || vel.y != 0) {
        angle = atan2(vel.y, vel.x);
    }
}

void Projectile::update(void) {
    if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
        syncFromPhysics();
    } else {
        pos.x += vel.x;
        pos.y += vel.y;
    }

    // Decrease lifetime
    lifetime -= 1.0f / 60.0f;  // Assuming 60 FPS

    // Check expiration conditions
    if (lifetime <= 0 || bounceCount >= maxBounces) {
        expired = true;
    }
}

void Projectile::draw(void) {
    if (expired) return;

    // Draw projectile as a small bright dot/circle
    float radius = width2;
    draw_filled_ellipse(pos.x, pos.y, radius, radius, color);

    // Draw a trail/tail
    float trailLen = 10.0f;
    float trailX = pos.x - cos(angle) * trailLen;
    float trailY = pos.y - sin(angle) * trailLen;
    GameColor trailColor = map_rgb(255, 200, 0);
    draw_line(pos.x, pos.y, trailX, trailY, trailColor, 2.0f);
}
