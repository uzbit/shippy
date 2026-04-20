#ifndef _PHYSICS_WORLD_H_
#define _PHYSICS_WORLD_H_

#include <box2d/box2d.h>
#include "defines.h"

class Object;

enum class PhysicsBodyType {
    DYNAMIC,
    STATIC,
    KINEMATIC,
    SENSOR
};

class PhysicsWorld {
public:
    PhysicsWorld();
    ~PhysicsWorld();

    void init(float gravity_y);
    void step(float dt);
    void destroy();

    b2BodyId createBody(Object* obj, PhysicsBodyType type, float density = 1.0f, float friction = 0.3f, float restitution = 0.1f);
    b2BodyId createCircleBody(Object* obj, PhysicsBodyType type, float radius, float density = 1.0f, float friction = 0.3f, float restitution = 0.1f);
    void destroyBody(b2BodyId bodyId);
    void enableBody(b2BodyId bodyId);
    void disableBody(b2BodyId bodyId);

    void applyForce(b2BodyId bodyId, float fx, float fy);
    void applyForceToCenter(b2BodyId bodyId, float fx, float fy);
    void setLinearVelocity(b2BodyId bodyId, float vx, float vy);
    void setTransform(b2BodyId bodyId, float x, float y, float angle);

    b2Vec2 getPosition(b2BodyId bodyId);
    b2Vec2 getLinearVelocity(b2BodyId bodyId);
    float getAngle(b2BodyId bodyId);

    static float toMeters(float px) { return px / PIXELS_PER_METER; }
    static float toPixels(float m) { return m * PIXELS_PER_METER; }

    b2WorldId getWorldId() const { return worldId; }
    bool isValid() const { return b2World_IsValid(worldId); }

private:
    b2WorldId worldId;
    bool initialized;
};

#endif
