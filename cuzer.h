#ifndef _CUZER_H_
#define _CUZER_H_

#include "object.h"
#include "sdl_compat.h"

class PhysicsWorld;

enum class CuzerSize {
    SMALL,   // 1 HP, smaller radius
    MEDIUM,  // 2 HP, medium radius
    LARGE    // 3 HP, larger radius
};

class Cuzer : public Object {
public:
    Cuzer();
    Cuzer(float x, float y, CuzerSize size);
    ~Cuzer() {}

    void update(void);
    void draw(void);
    void initPhysics(PhysicsWorld& world) override;
    void syncFromPhysics() override;
    void setPhysicsWorld(PhysicsWorld* world) { physicsWorldPtr = world; }

    bool isDestroyed() const { return destroyed; }
    void destroy() { destroyed = true; }
    bool takeHit();  // Returns true if destroyed (HP reaches 0)
    CuzerSize getSize() const { return size; }
    float getRadius() const { return radius; }
    GameColor getColor() const { return baseColor; }

    Point vel;
    float angle;
    float angularVel;
    bool destroyed;

private:
    CuzerSize size;
    float radius;
    int hitPoints;
    int maxHitPoints;
    GameColor baseColor;
    PhysicsWorld* physicsWorldPtr = nullptr;
};

#endif
