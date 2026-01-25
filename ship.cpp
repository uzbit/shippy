
#include <SDL3/SDL.h>
#include <iostream>
#include <math.h>

#include "ship.h"
#include "space.h"
#include "body.h"
#include "geom.h"
#include "defines.h"
#include "sdl_compat.h"
#include "physics_world.h"

using namespace std;

Ship::Ship(float x, float y, float fuel, float mass)
:Object(x, y, 10, 40), fuel(fuel), mass(mass), thick(4){
    pos.x = x;
    pos.y = y;
    prev_pos.x = x;
    prev_pos.y = y;
    thrustPower = THRUST_Y;
    rotatePower = 0.005f;  // Angular acceleration for rotation
    offset = SDL_GetTicks() / 1000.0f;
    fuel_start = fuel;
    angle = -M_PI / 2;  // Start pointing up
    angularVelocity = 0;
    isThrusting = false;
    isBraking = false;
    thrustFlameCounter = 0;
    brakeFlameCounter = 0;
}

void Ship::rotate(float scale){
    // Apply torque for rotation (no fuel cost for rotation)
    if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
        float torque = scale * rotatePower * 60.0f;
        b2Body_ApplyTorque(physicsBody, torque, true);
    } else {
        angularVelocity += scale * rotatePower * 0.01f;
    }
}

void Ship::thrust(float scale){
    if (fuel > 0){
        float force = scale * thrustPower / (mass + fuel * FUEL_MASS);
        // Thrust in the direction the ship is facing
        float fx = cos(angle) * force;
        float fy = sin(angle) * force;
        if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
            physicsWorldPtr->applyForceToCenter(physicsBody, fx * PIXELS_PER_METER * 60.0f, fy * PIXELS_PER_METER * 60.0f);
        } else {
            accel.x += fx;
            accel.y += fy;
        }
        fuel -= VERTICAL_FUEL_CONSUMPTION;
        isThrusting = true;
        thrustFlameCounter = 0;
    }
    if (fuel < 0){
        fuel = 0;
        isThrusting = false;
    }
}

void Ship::brake(float scale){
    if (fuel > 0){
        // Brake by applying force opposite to current velocity
        float speed = sqrt(vel.x * vel.x + vel.y * vel.y);
        if (speed > 0.1f) {
            float brakeForce = scale * thrustPower * 0.5f / (mass + fuel * FUEL_MASS);
            float fx = -(vel.x / speed) * brakeForce;
            float fy = -(vel.y / speed) * brakeForce;
            if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
                physicsWorldPtr->applyForceToCenter(physicsBody, fx * PIXELS_PER_METER * 60.0f, fy * PIXELS_PER_METER * 60.0f);
            } else {
                accel.x += fx;
                accel.y += fy;
            }
            fuel -= HORIZONTAL_FUEL_CONSUMPTION;
            isBraking = true;
            brakeFlameCounter = 0;
        }
    }
    if (fuel < 0){
        fuel = 0;
        isBraking = false;
    }
}

void Ship::gravitate_bodies(Space &space){
    if (!space.gravitate_bodies) return;

    float G = 0.05;
    float mass, dist, dx, dy;
    int sign = 1;
    for (int i = 0; i < space.body_count; i++){
        if (space.coordy == 0 && i == 0) continue; // already gravitate to Earth
        Body *b = space.bodies[i];
        mass = (b->width * b->height) * b->density;
        dx = b->pos.x - pos.x;
        dy = b->pos.y - pos.y;
        dist = sqrt(dx*dx + dy*dy);
        accel.x += sign * dx * mass * G / (dist*dist*dist);
        accel.y += sign * dy * mass * G / (dist*dist*dist);

    }
}

void Ship::update(void){
    if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
        syncFromPhysics();
    } else {
        pos.x += vel.x;
        pos.y += vel.y;
        vel.x += accel.x;
        vel.y += 0.01 + accel.y; //gravity = 0.01
        accel.x = 0;
        accel.y = 0;
    }
}

void Ship::initPhysics(PhysicsWorld& world) {
    physicsWorldPtr = &world;
    physicsBody = world.createBody(this, PhysicsBodyType::DYNAMIC, 1.0f, 0.3f, BOUNCE_FACTOR);
    if (b2Body_IsValid(physicsBody)) {
        b2Body_SetLinearDamping(physicsBody, 0.0f);
        b2Body_SetAngularDamping(physicsBody, 3.0f);  // Angular damping for smooth rotation feel
        b2Body_SetFixedRotation(physicsBody, false);  // Enable rotation
        // Set initial rotation
        b2Vec2 position = b2Body_GetPosition(physicsBody);
        b2Rot rotation = b2MakeRot(angle);
        b2Body_SetTransform(physicsBody, position, rotation);
    }
}

void Ship::syncFromPhysics() {
    if (!physicsWorldPtr || !b2Body_IsValid(physicsBody)) return;

    b2Vec2 physPos = physicsWorldPtr->getPosition(physicsBody);
    b2Vec2 physVel = physicsWorldPtr->getLinearVelocity(physicsBody);
    b2Rot rotation = b2Body_GetRotation(physicsBody);

    pos.x = physPos.x;
    pos.y = physPos.y;
    vel.x = physVel.x;
    vel.y = physVel.y;
    angle = b2Rot_GetAngle(rotation);
    angularVelocity = b2Body_GetAngularVelocity(physicsBody);
}

// Draw a flame triangle with manual rotation
void Ship::draw_flame(float tx, float ty, float scale, float angle){
    // Original flame triangle vertices (centered at origin, pointing left)
    float p1x = -15, p1y = 0;
    float p2x = 0, p2y = width2;
    float p3x = 0, p3y = -width2;

    // Apply scale
    p1x *= scale; p1y *= scale;
    p2x *= scale; p2y *= scale;
    p3x *= scale; p3y *= scale;

    // Apply rotation
    float cosA = cos(angle);
    float sinA = sin(angle);

    float r1x = p1x * cosA - p1y * sinA;
    float r1y = p1x * sinA + p1y * cosA;
    float r2x = p2x * cosA - p2y * sinA;
    float r2y = p2x * sinA + p2y * cosA;
    float r3x = p3x * cosA - p3y * sinA;
    float r3y = p3x * sinA + p3y * cosA;

    // Translate to position
    r1x += tx; r1y += ty;
    r2x += tx; r2y += ty;
    r3x += tx; r3y += ty;

    GameColor flame_color = map_rgb(25, 200, 2);
    draw_filled_triangle(r1x, r1y, r2x, r2y, r3x, r3y, flame_color);
}

void Ship::draw_thrust_flame(void){
    if (isThrusting){
        // Draw flame behind the ship (opposite to facing direction)
        // Position flame behind ship, but point it in ship's facing direction
        // so the exhaust extends backward (away from the ship)
        float flameAngle = angle;  // Point in ship's facing direction
        float flameDist = height2 + thick/2;
        float tx = pos.x + cos(angle + M_PI) * flameDist;  // Position behind ship
        float ty = pos.y + sin(angle + M_PI) * flameDist;
        draw_flame(tx, ty, 1.0f, flameAngle);
        thrustFlameCounter++;
        if (thrustFlameCounter >= 3) {
            isThrusting = false;
        }
    }
}

void Ship::draw_brake_flame(void){
    if (isBraking){
        // Draw small flames on the sides when braking
        float flameDist = height2 + thick/2;
        float tx = pos.x + cos(angle) * flameDist;
        float ty = pos.y + sin(angle) * flameDist;
        draw_flame(tx, ty, 0.5f, angle);
        brakeFlameCounter++;
        if (brakeFlameCounter >= 3) {
            isBraking = false;
        }
    }
}

void Ship::draw(void){
    computeRect();

    // Draw thrust flames
    draw_thrust_flame();
    draw_brake_flame();

    // Draw rotated ship body as a triangle pointing in facing direction
    GameColor ship_color = map_rgb(2, 255, 255);

    // Ship is a triangle: nose at front, two points at back
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Triangle vertices (relative to center, pointing right when angle=0)
    // Nose point (front)
    float nose_len = height2;
    float nx = pos.x + cosA * nose_len;
    float ny = pos.y + sinA * nose_len;

    // Back left point
    float back_len = height2;
    float wing_spread = width2 * 1.5f;
    float blx = pos.x - cosA * back_len - sinA * wing_spread;
    float bly = pos.y - sinA * back_len + cosA * wing_spread;

    // Back right point
    float brx = pos.x - cosA * back_len + sinA * wing_spread;
    float bry = pos.y - sinA * back_len - cosA * wing_spread;

    // Draw ship outline as triangle
    draw_line(nx, ny, blx, bly, ship_color, thick);
    draw_line(blx, bly, brx, bry, ship_color, thick);
    draw_line(brx, bry, nx, ny, ship_color, thick);

    // Draw fuel gauge as a line inside the ship
    if (fuel > 0){
        float fuel_ratio = fuel / fuel_start;
        GameColor fuel_color = map_rgb(255, 30, 2);

        // Draw fuel as a line from back to front, length proportional to fuel
        float gauge_len = height2 * 0.8f * fuel_ratio;
        float gauge_start_x = pos.x - cosA * (height2 * 0.4f);
        float gauge_start_y = pos.y - sinA * (height2 * 0.4f);
        float gauge_end_x = gauge_start_x + cosA * gauge_len;
        float gauge_end_y = gauge_start_y + sinA * gauge_len;

        draw_line(gauge_start_x, gauge_start_y, gauge_end_x, gauge_end_y, fuel_color, thick * 0.5f);
    }
}
