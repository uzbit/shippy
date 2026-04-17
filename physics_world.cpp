#include "physics_world.h"
#include "object.h"
#include <iostream>

PhysicsWorld::PhysicsWorld() : initialized(false) {
    worldId = b2_nullWorldId;
}

PhysicsWorld::~PhysicsWorld() {
    destroy();
}

void PhysicsWorld::init(float gravity_y) {
    if (initialized) {
        destroy();
    }

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, toMeters(gravity_y)};
    worldId = b2CreateWorld(&worldDef);
    initialized = true;
}

void PhysicsWorld::destroy() {
    if (initialized && b2World_IsValid(worldId)) {
        b2DestroyWorld(worldId);
        worldId = b2_nullWorldId;
        initialized = false;
    }
}

void PhysicsWorld::step(float dt) {
    if (!initialized) return;

    int subStepCount = 4;
    b2World_Step(worldId, dt, subStepCount);
}

b2BodyId PhysicsWorld::createBody(Object* obj, PhysicsBodyType type, float density, float friction, float restitution) {
    if (!initialized) return b2_nullBodyId;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.position = {toMeters(obj->pos.x), toMeters(obj->pos.y)};

    switch (type) {
        case PhysicsBodyType::DYNAMIC:
            bodyDef.type = b2_dynamicBody;
            break;
        case PhysicsBodyType::STATIC:
        case PhysicsBodyType::SENSOR:
            bodyDef.type = b2_staticBody;
            break;
        case PhysicsBodyType::KINEMATIC:
            bodyDef.type = b2_kinematicBody;
            break;
    }

    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);
    b2Body_SetUserData(bodyId, obj);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = density;
    shapeDef.material.friction = friction;
    shapeDef.material.restitution = restitution;
    shapeDef.userData = obj;

    if (type == PhysicsBodyType::SENSOR) {
        shapeDef.isSensor = true;
        shapeDef.enableSensorEvents = true;
    }

    // Enable sensor events for dynamic bodies so they can trigger sensors
    // Enable contact events for dynamic and kinematic bodies for collision callbacks
    if (type == PhysicsBodyType::DYNAMIC) {
        shapeDef.enableSensorEvents = true;
        shapeDef.enableContactEvents = true;
        shapeDef.enableHitEvents = true;
    }

    if (type == PhysicsBodyType::KINEMATIC) {
        shapeDef.enableContactEvents = true;
    }

    b2Polygon box = b2MakeBox(toMeters(obj->width2), toMeters(obj->height2));
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    return bodyId;
}

void PhysicsWorld::destroyBody(b2BodyId bodyId) {
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
    }
}

void PhysicsWorld::enableBody(b2BodyId bodyId) {
    if (b2Body_IsValid(bodyId)) {
        b2Body_Enable(bodyId);
    }
}

void PhysicsWorld::disableBody(b2BodyId bodyId) {
    if (b2Body_IsValid(bodyId)) {
        b2Body_Disable(bodyId);
    }
}

void PhysicsWorld::applyForce(b2BodyId bodyId, float fx, float fy) {
    if (!b2Body_IsValid(bodyId)) return;
    b2Vec2 force = {toMeters(fx), toMeters(fy)};
    b2Vec2 point = b2Body_GetPosition(bodyId);
    b2Body_ApplyForce(bodyId, force, point, true);
}

void PhysicsWorld::applyForceToCenter(b2BodyId bodyId, float fx, float fy) {
    if (!b2Body_IsValid(bodyId)) return;
    b2Vec2 force = {toMeters(fx), toMeters(fy)};
    b2Body_ApplyForceToCenter(bodyId, force, true);
}

void PhysicsWorld::setLinearVelocity(b2BodyId bodyId, float vx, float vy) {
    if (!b2Body_IsValid(bodyId)) return;
    b2Vec2 velocity = {toMeters(vx), toMeters(vy)};
    b2Body_SetLinearVelocity(bodyId, velocity);
}

void PhysicsWorld::setTransform(b2BodyId bodyId, float x, float y, float angle) {
    if (!b2Body_IsValid(bodyId)) return;
    b2Vec2 position = {toMeters(x), toMeters(y)};
    b2Rot rotation = b2MakeRot(angle);
    b2Body_SetTransform(bodyId, position, rotation);
}

b2Vec2 PhysicsWorld::getPosition(b2BodyId bodyId) {
    if (!b2Body_IsValid(bodyId)) return {0, 0};
    b2Vec2 pos = b2Body_GetPosition(bodyId);
    return {toPixels(pos.x), toPixels(pos.y)};
}

b2Vec2 PhysicsWorld::getLinearVelocity(b2BodyId bodyId) {
    if (!b2Body_IsValid(bodyId)) return {0, 0};
    b2Vec2 vel = b2Body_GetLinearVelocity(bodyId);
    return {toPixels(vel.x), toPixels(vel.y)};
}

float PhysicsWorld::getAngle(b2BodyId bodyId) {
    if (!b2Body_IsValid(bodyId)) return 0.0f;
    b2Rot rot = b2Body_GetRotation(bodyId);
    return b2Rot_GetAngle(rot);
}
