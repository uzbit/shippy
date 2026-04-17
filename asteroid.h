#ifndef _ASTEROID_H_
#define _ASTEROID_H_

#include "object.h"
#include "sdl_compat.h"

class PhysicsWorld;

enum class AsteroidSize {
    SMALL,   // 20-30px, destroyed completely
    MEDIUM,  // 40-60px, splits into 2-3 small
    LARGE    // 80-120px, splits into 2-3 medium
};

class Asteroid : public Object {
public:
    Asteroid();
    Asteroid(float x, float y, AsteroidSize size);
    ~Asteroid() {}

    void update(void);
    void draw(void);
    void initPhysics(PhysicsWorld& world) override;
    void syncFromPhysics() override;
    void setPhysicsWorld(PhysicsWorld* world) { physicsWorldPtr = world; }

    bool isDestroyed() const { return destroyed; }
    void destroy() { destroyed = true; }
    AsteroidSize getSize() const { return size; }
    float getRadius() const { return radius; }
    GameColor getColor() const { return color; }

    Point vel;
    float angle;
    float angularVel;
    bool destroyed;

private:
    AsteroidSize size;
    float radius;
    GameColor color;
    PhysicsWorld* physicsWorldPtr = nullptr;
    int numVertices;
    float vertices[12];  // Irregular shape vertices (up to 12 points)
};

#endif
