#ifndef _PROJECTILE_H_
#define _PROJECTILE_H_

#include "object.h"
#include "sdl_compat.h"

class PhysicsWorld;

class Projectile : public Object {
public:
    Projectile();
    Projectile(float x, float y, float angle, float speed);
    ~Projectile() {}

    void update(void);
    void draw(void);
    void initPhysics(PhysicsWorld& world) override;
    void syncFromPhysics() override;
    void setPhysicsWorld(PhysicsWorld* world) { physicsWorldPtr = world; }

    bool isExpired() const { return expired; }
    int getBounceCount() const { return bounceCount; }
    void incrementBounce() { bounceCount++; }

    Point vel;
    float angle;
    float lifetime;      // Time remaining before expiration
    int bounceCount;     // Number of bounces
    int maxBounces;      // Maximum bounces before expiration
    bool expired;

private:
    float speed;
    GameColor color;
    PhysicsWorld* physicsWorldPtr = nullptr;
};

#endif
