
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
:Object(x, y, 30, 80), fuel(fuel), mass(mass), thick(5){
    pos.x = x;
    pos.y = y;
    prev_pos.x = x;
    prev_pos.y = y;
    thrustPower = THRUST_Y;
    rotatePower = 0.05f;  // Angular acceleration for rotation
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

    float cosA = cos(angle);
    float sinA = sin(angle);

    // Ship dimensions
    float nose_len = height2;
    float back_len = height2 * 0.7f;
    float wing_spread = width2 * 1.8f;
    float engine_size = width2 * 0.8f;

    // Main body triangle vertices
    // Nose point (front)
    float nx = pos.x + cosA * nose_len;
    float ny = pos.y + sinA * nose_len;

    // Back left point
    float blx = pos.x - cosA * back_len - sinA * wing_spread;
    float bly = pos.y - sinA * back_len + cosA * wing_spread;

    // Back right point
    float brx = pos.x - cosA * back_len + sinA * wing_spread;
    float bry = pos.y - sinA * back_len - cosA * wing_spread;

    // Engine positions (at the back corners)
    float engine_offset = back_len + engine_size * 0.3f;
    float engine_left_x = pos.x - cosA * engine_offset - sinA * (wing_spread * 0.6f);
    float engine_left_y = pos.y - sinA * engine_offset + cosA * (wing_spread * 0.6f);
    float engine_right_x = pos.x - cosA * engine_offset + sinA * (wing_spread * 0.6f);
    float engine_right_y = pos.y - sinA * engine_offset - cosA * (wing_spread * 0.6f);

    // Draw engine flames first (behind everything)
    if (isThrusting) {
        GameColor flame_color = map_rgb(255, 150, 20);
        GameColor flame_core = map_rgb(255, 255, 100);

        // Flame size varies for effect - made bigger
        float flame_len = engine_size * (2.5f + 1.0f * ((thrustFlameCounter % 3) / 2.0f));

        // Left engine flame
        float fl_tip_x = engine_left_x - cosA * flame_len;
        float fl_tip_y = engine_left_y - sinA * flame_len;
        float fl_l_x = engine_left_x - sinA * (engine_size * 0.6f);
        float fl_l_y = engine_left_y + cosA * (engine_size * 0.6f);
        float fl_r_x = engine_left_x + sinA * (engine_size * 0.6f);
        float fl_r_y = engine_left_y - cosA * (engine_size * 0.6f);
        draw_filled_triangle(fl_tip_x, fl_tip_y, fl_l_x, fl_l_y, fl_r_x, fl_r_y, flame_color);

        // Left engine flame core
        float flc_tip_x = engine_left_x - cosA * (flame_len * 0.7f);
        float flc_tip_y = engine_left_y - sinA * (flame_len * 0.7f);
        float flc_l_x = engine_left_x - sinA * (engine_size * 0.3f);
        float flc_l_y = engine_left_y + cosA * (engine_size * 0.3f);
        float flc_r_x = engine_left_x + sinA * (engine_size * 0.3f);
        float flc_r_y = engine_left_y - cosA * (engine_size * 0.3f);
        draw_filled_triangle(flc_tip_x, flc_tip_y, flc_l_x, flc_l_y, flc_r_x, flc_r_y, flame_core);

        // Right engine flame
        float fr_tip_x = engine_right_x - cosA * flame_len;
        float fr_tip_y = engine_right_y - sinA * flame_len;
        float fr_l_x = engine_right_x - sinA * (engine_size * 0.6f);
        float fr_l_y = engine_right_y + cosA * (engine_size * 0.6f);
        float fr_r_x = engine_right_x + sinA * (engine_size * 0.6f);
        float fr_r_y = engine_right_y - cosA * (engine_size * 0.6f);
        draw_filled_triangle(fr_tip_x, fr_tip_y, fr_l_x, fr_l_y, fr_r_x, fr_r_y, flame_color);

        // Right engine flame core
        float frc_tip_x = engine_right_x - cosA * (flame_len * 0.7f);
        float frc_tip_y = engine_right_y - sinA * (flame_len * 0.7f);
        float frc_l_x = engine_right_x - sinA * (engine_size * 0.3f);
        float frc_l_y = engine_right_y + cosA * (engine_size * 0.3f);
        float frc_r_x = engine_right_x + sinA * (engine_size * 0.3f);
        float frc_r_y = engine_right_y - cosA * (engine_size * 0.3f);
        draw_filled_triangle(frc_tip_x, frc_tip_y, frc_l_x, frc_l_y, frc_r_x, frc_r_y, flame_core);

        thrustFlameCounter++;
        if (thrustFlameCounter >= 6) {
            isThrusting = false;
        }
    }

    // Draw brake flames (front of ship)
    if (isBraking) {
        GameColor brake_flame = map_rgb(100, 200, 255);
        float brake_len = engine_size * 0.8f;

        // Brake flame at nose
        float bf_tip_x = nx + cosA * brake_len;
        float bf_tip_y = ny + sinA * brake_len;
        float bf_l_x = nx - sinA * (engine_size * 0.3f);
        float bf_l_y = ny + cosA * (engine_size * 0.3f);
        float bf_r_x = nx + sinA * (engine_size * 0.3f);
        float bf_r_y = ny - cosA * (engine_size * 0.3f);
        draw_filled_triangle(bf_tip_x, bf_tip_y, bf_l_x, bf_l_y, bf_r_x, bf_r_y, brake_flame);

        brakeFlameCounter++;
        if (brakeFlameCounter >= 4) {
            isBraking = false;
        }
    }

    // Draw engines (small triangles at back)
    GameColor engine_color = map_rgb(150, 150, 180);

    // Left engine triangle
    float el_nose_x = engine_left_x + cosA * (engine_size * 0.3f);
    float el_nose_y = engine_left_y + sinA * (engine_size * 0.3f);
    float el_l_x = engine_left_x - cosA * (engine_size * 0.5f) - sinA * (engine_size * 0.5f);
    float el_l_y = engine_left_y - sinA * (engine_size * 0.5f) + cosA * (engine_size * 0.5f);
    float el_r_x = engine_left_x - cosA * (engine_size * 0.5f) + sinA * (engine_size * 0.5f);
    float el_r_y = engine_left_y - sinA * (engine_size * 0.5f) - cosA * (engine_size * 0.5f);
    draw_filled_triangle(el_nose_x, el_nose_y, el_l_x, el_l_y, el_r_x, el_r_y, engine_color);

    // Right engine triangle
    float er_nose_x = engine_right_x + cosA * (engine_size * 0.3f);
    float er_nose_y = engine_right_y + sinA * (engine_size * 0.3f);
    float er_l_x = engine_right_x - cosA * (engine_size * 0.5f) - sinA * (engine_size * 0.5f);
    float er_l_y = engine_right_y - sinA * (engine_size * 0.5f) + cosA * (engine_size * 0.5f);
    float er_r_x = engine_right_x - cosA * (engine_size * 0.5f) + sinA * (engine_size * 0.5f);
    float er_r_y = engine_right_y - sinA * (engine_size * 0.5f) - cosA * (engine_size * 0.5f);
    draw_filled_triangle(er_nose_x, er_nose_y, er_l_x, er_l_y, er_r_x, er_r_y, engine_color);

    // Draw main body triangle (filled)
    GameColor body_color = map_rgb(40, 80, 120);
    draw_filled_triangle(nx, ny, blx, bly, brx, bry, body_color);

    // Draw main body outline
    GameColor ship_color = map_rgb(2, 255, 255);
    draw_line(nx, ny, blx, bly, ship_color, thick);
    draw_line(blx, bly, brx, bry, ship_color, thick);
    draw_line(brx, bry, nx, ny, ship_color, thick);

    // Draw cockpit (small triangle at front)
    GameColor cockpit_color = map_rgb(100, 200, 255);
    float cockpit_size = height2 * 0.3f;
    float cp_nose_x = pos.x + cosA * (nose_len * 0.7f);
    float cp_nose_y = pos.y + sinA * (nose_len * 0.7f);
    float cp_l_x = pos.x + cosA * (nose_len * 0.2f) - sinA * (cockpit_size * 0.4f);
    float cp_l_y = pos.y + sinA * (nose_len * 0.2f) + cosA * (cockpit_size * 0.4f);
    float cp_r_x = pos.x + cosA * (nose_len * 0.2f) + sinA * (cockpit_size * 0.4f);
    float cp_r_y = pos.y + sinA * (nose_len * 0.2f) - cosA * (cockpit_size * 0.4f);
    draw_filled_triangle(cp_nose_x, cp_nose_y, cp_l_x, cp_l_y, cp_r_x, cp_r_y, cockpit_color);

    // Draw fuel gauge as a bar inside the ship
    if (fuel > 0){
        float fuel_ratio = fuel / fuel_start;
        // Color changes from green to yellow to red as fuel depletes
        GameColor fuel_color;
        if (fuel_ratio > 0.5f) {
            fuel_color = map_rgb(50, 255, 50);  // Green
        } else if (fuel_ratio > 0.25f) {
            fuel_color = map_rgb(255, 255, 50);  // Yellow
        } else {
            fuel_color = map_rgb(255, 50, 50);  // Red
        }

        // Draw fuel as a thicker line from back to front
        float gauge_len = height2 * 0.6f * fuel_ratio;
        float gauge_start_x = pos.x - cosA * (height2 * 0.35f);
        float gauge_start_y = pos.y - sinA * (height2 * 0.35f);
        float gauge_end_x = gauge_start_x + cosA * gauge_len;
        float gauge_end_y = gauge_start_y + sinA * gauge_len;

        draw_line(gauge_start_x, gauge_start_y, gauge_end_x, gauge_end_y, fuel_color, thick * 1.2f);
    }
}
