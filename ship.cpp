
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

    // Compute ship geometry once
    body_length = height2 * 1.6f;
    body_width = width2 * 0.7f;
    dome_radius = body_width * 1.1f;
    engine_radius = body_width * 0.8f;
    engine_spread = body_width * 1.6f;
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

    // Create body without default shape - we'll add custom compound shape
    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {world.toMeters(pos.x), world.toMeters(pos.y)};

    physicsBody = b2CreateBody(world.getWorldId(), &bodyDef);
    b2Body_SetUserData(physicsBody, this);

    if (b2Body_IsValid(physicsBody)) {
        b2ShapeDef shapeDef = b2DefaultShapeDef();
        shapeDef.density = 1.0f;
        shapeDef.material.friction = 0.3f;
        shapeDef.material.restitution = BOUNCE_FACTOR;
        shapeDef.enableSensorEvents = true;
        shapeDef.enableContactEvents = true;
        shapeDef.userData = this;

        // Main body (rectangle) - centered slightly back from object center
        float body_center_offset = -body_length * 0.1f;  // Slightly back
        b2Polygon bodyBox = b2MakeOffsetBox(
            world.toMeters(body_width),
            world.toMeters(body_length * 0.4f),
            {world.toMeters(body_center_offset), 0},
            b2MakeRot(0)
        );
        b2CreatePolygonShape(physicsBody, &shapeDef, &bodyBox);

        // Dome (circle at front)
        float dome_offset = body_length * 0.5f;
        b2Circle domeCircle;
        domeCircle.center = {world.toMeters(dome_offset), 0};
        domeCircle.radius = world.toMeters(dome_radius);
        b2CreateCircleShape(physicsBody, &shapeDef, &domeCircle);

        // Left engine pod (circle)
        float engine_back_offset = -body_length * 0.5f;
        b2Circle leftEngine;
        leftEngine.center = {world.toMeters(engine_back_offset), world.toMeters(-engine_spread)};
        leftEngine.radius = world.toMeters(engine_radius);
        b2CreateCircleShape(physicsBody, &shapeDef, &leftEngine);

        // Right engine pod (circle)
        b2Circle rightEngine;
        rightEngine.center = {world.toMeters(engine_back_offset), world.toMeters(engine_spread)};
        rightEngine.radius = world.toMeters(engine_radius);
        b2CreateCircleShape(physicsBody, &shapeDef, &rightEngine);

        b2Body_SetLinearDamping(physicsBody, 0.0f);
        b2Body_SetAngularDamping(physicsBody, 3.0f);
        b2Body_SetFixedRotation(physicsBody, false);

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
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Body positions
    float body_front_x = pos.x + cosA * (body_length * 0.3f);
    float body_front_y = pos.y + sinA * (body_length * 0.3f);
    float body_back_x = pos.x - cosA * (body_length * 0.5f);
    float body_back_y = pos.y - sinA * (body_length * 0.5f);

    // Engine pod positions
    float engine_left_x = body_back_x - sinA * engine_spread;
    float engine_left_y = body_back_y + cosA * engine_spread;
    float engine_right_x = body_back_x + sinA * engine_spread;
    float engine_right_y = body_back_y - cosA * engine_spread;

    // Dome position
    float dome_x = pos.x + cosA * (body_length * 0.5f);
    float dome_y = pos.y + sinA * (body_length * 0.5f);

    // Draw engine flames first (behind everything)
    if (isThrusting) {
        GameColor flame_color = map_rgb(255, 150, 20);
        GameColor flame_core = map_rgb(255, 255, 100);
        float flame_len = engine_radius * (3.0f + 1.5f * ((thrustFlameCounter % 4) / 3.0f));

        // Left engine flame
        float fl_tip_x = engine_left_x - cosA * flame_len;
        float fl_tip_y = engine_left_y - sinA * flame_len;
        float fl_l_x = engine_left_x - sinA * (engine_radius * 0.8f);
        float fl_l_y = engine_left_y + cosA * (engine_radius * 0.8f);
        float fl_r_x = engine_left_x + sinA * (engine_radius * 0.8f);
        float fl_r_y = engine_left_y - cosA * (engine_radius * 0.8f);
        draw_filled_triangle(fl_tip_x, fl_tip_y, fl_l_x, fl_l_y, fl_r_x, fl_r_y, flame_color);

        // Left flame core
        float flc_tip_x = engine_left_x - cosA * (flame_len * 0.7f);
        float flc_tip_y = engine_left_y - sinA * (flame_len * 0.7f);
        float flc_l_x = engine_left_x - sinA * (engine_radius * 0.4f);
        float flc_l_y = engine_left_y + cosA * (engine_radius * 0.4f);
        float flc_r_x = engine_left_x + sinA * (engine_radius * 0.4f);
        float flc_r_y = engine_left_y - cosA * (engine_radius * 0.4f);
        draw_filled_triangle(flc_tip_x, flc_tip_y, flc_l_x, flc_l_y, flc_r_x, flc_r_y, flame_core);

        // Right engine flame
        float fr_tip_x = engine_right_x - cosA * flame_len;
        float fr_tip_y = engine_right_y - sinA * flame_len;
        float fr_l_x = engine_right_x - sinA * (engine_radius * 0.8f);
        float fr_l_y = engine_right_y + cosA * (engine_radius * 0.8f);
        float fr_r_x = engine_right_x + sinA * (engine_radius * 0.8f);
        float fr_r_y = engine_right_y - cosA * (engine_radius * 0.8f);
        draw_filled_triangle(fr_tip_x, fr_tip_y, fr_l_x, fr_l_y, fr_r_x, fr_r_y, flame_color);

        // Right flame core
        float frc_tip_x = engine_right_x - cosA * (flame_len * 0.7f);
        float frc_tip_y = engine_right_y - sinA * (flame_len * 0.7f);
        float frc_l_x = engine_right_x - sinA * (engine_radius * 0.4f);
        float frc_l_y = engine_right_y + cosA * (engine_radius * 0.4f);
        float frc_r_x = engine_right_x + sinA * (engine_radius * 0.4f);
        float frc_r_y = engine_right_y - cosA * (engine_radius * 0.4f);
        draw_filled_triangle(frc_tip_x, frc_tip_y, frc_l_x, frc_l_y, frc_r_x, frc_r_y, flame_core);

        thrustFlameCounter++;
        if (thrustFlameCounter >= 6) {
            isThrusting = false;
        }
    }

    // Draw brake flames
    if (isBraking) {
        GameColor brake_flame = map_rgb(100, 200, 255);
        float brake_len = dome_radius * 1.2f;
        float bf_tip_x = dome_x + cosA * brake_len;
        float bf_tip_y = dome_y + sinA * brake_len;
        float bf_l_x = dome_x - sinA * (dome_radius * 0.5f);
        float bf_l_y = dome_y + cosA * (dome_radius * 0.5f);
        float bf_r_x = dome_x + sinA * (dome_radius * 0.5f);
        float bf_r_y = dome_y - cosA * (dome_radius * 0.5f);
        draw_filled_triangle(bf_tip_x, bf_tip_y, bf_l_x, bf_l_y, bf_r_x, bf_r_y, brake_flame);

        brakeFlameCounter++;
        if (brakeFlameCounter >= 4) {
            isBraking = false;
        }
    }

    // Draw engine pods (semi-spheres)
    GameColor engine_color = map_rgb(80, 80, 100);
    GameColor engine_highlight = map_rgb(120, 120, 150);

    // Left engine pod
    draw_filled_ellipse(engine_left_x, engine_left_y, engine_radius, engine_radius, engine_color);
    draw_filled_ellipse(engine_left_x + cosA * (engine_radius * 0.2f),
                        engine_left_y + sinA * (engine_radius * 0.2f),
                        engine_radius * 0.5f, engine_radius * 0.5f, engine_highlight);

    // Right engine pod
    draw_filled_ellipse(engine_right_x, engine_right_y, engine_radius, engine_radius, engine_color);
    draw_filled_ellipse(engine_right_x + cosA * (engine_radius * 0.2f),
                        engine_right_y + sinA * (engine_radius * 0.2f),
                        engine_radius * 0.5f, engine_radius * 0.5f, engine_highlight);

    // Draw cylinder body (as a quad)
    GameColor body_color = map_rgb(60, 100, 140);
    GameColor body_outline = map_rgb(2, 255, 255);

    float bl_x = body_back_x - sinA * body_width;
    float bl_y = body_back_y + cosA * body_width;
    float br_x = body_back_x + sinA * body_width;
    float br_y = body_back_y - cosA * body_width;
    float fl_x = body_front_x - sinA * body_width;
    float fl_y = body_front_y + cosA * body_width;
    float fr_x = body_front_x + sinA * body_width;
    float fr_y = body_front_y - cosA * body_width;

    draw_filled_triangle(bl_x, bl_y, br_x, br_y, fl_x, fl_y, body_color);
    draw_filled_triangle(br_x, br_y, fr_x, fr_y, fl_x, fl_y, body_color);

    // Body outline
    draw_line(bl_x, bl_y, fl_x, fl_y, body_outline, thick);
    draw_line(br_x, br_y, fr_x, fr_y, body_outline, thick);
    draw_line(bl_x, bl_y, br_x, br_y, body_outline, thick);

    // Draw dome cockpit
    GameColor dome_color = map_rgb(100, 180, 220);
    GameColor dome_highlight = map_rgb(180, 230, 255);

    draw_filled_ellipse(dome_x, dome_y, dome_radius, dome_radius, dome_color);
    draw_filled_ellipse(dome_x + cosA * (dome_radius * 0.25f) - sinA * (dome_radius * 0.15f),
                        dome_y + sinA * (dome_radius * 0.25f) + cosA * (dome_radius * 0.15f),
                        dome_radius * 0.35f, dome_radius * 0.35f, dome_highlight);

    // Draw struts connecting body to engines
    GameColor strut_color = map_rgb(100, 100, 120);
    draw_line(bl_x, bl_y, engine_left_x, engine_left_y, strut_color, thick * 0.8f);
    draw_line(br_x, br_y, engine_right_x, engine_right_y, strut_color, thick * 0.8f);

    // Draw fuel gauge on body
    if (fuel > 0){
        float fuel_ratio = fuel / fuel_start;
        GameColor fuel_color;
        if (fuel_ratio > 0.5f) {
            fuel_color = map_rgb(50, 255, 50);
        } else if (fuel_ratio > 0.25f) {
            fuel_color = map_rgb(255, 255, 50);
        } else {
            fuel_color = map_rgb(255, 50, 50);
        }

        float gauge_len = body_length * 0.5f * fuel_ratio;
        float gauge_start_x = pos.x - cosA * (body_length * 0.3f);
        float gauge_start_y = pos.y - sinA * (body_length * 0.3f);
        float gauge_end_x = gauge_start_x + cosA * gauge_len;
        float gauge_end_y = gauge_start_y + sinA * gauge_len;

        draw_line(gauge_start_x, gauge_start_y, gauge_end_x, gauge_end_y, fuel_color, thick * 1.5f);
    }
}
