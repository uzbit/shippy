#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>

#include "cuzer.h"
#include "physics_world.h"
#include "defines.h"

Cuzer::Cuzer()
    : Object(0, 0, 40, 40), angle(0), angularVel(0), destroyed(false),
      size(CuzerSize::MEDIUM), radius(28), hitPoints(2), maxHitPoints(2),
      baseColor(map_rgb(100, 200, 255)) {
}

Cuzer::Cuzer(float x, float y, CuzerSize sz)
    : Object(x, y, 40, 40), angle(0), destroyed(false), size(sz),
      baseColor(map_rgb(100 + rand() % 50, 180 + rand() % 50, 220 + rand() % 35)) {

    // Set radius and hit points based on size
    switch (size) {
        case CuzerSize::SMALL:
            radius = 12 + rand() % 9;   // 12-20
            hitPoints = 1;
            maxHitPoints = 1;
            break;
        case CuzerSize::MEDIUM:
            radius = 22 + rand() % 13;  // 22-34
            hitPoints = 2;
            maxHitPoints = 2;
            break;
        case CuzerSize::LARGE:
            radius = 35 + rand() % 21;  // 35-55
            hitPoints = 3;
            maxHitPoints = 3;
            break;
    }

    width = radius * 2;
    height = radius * 2;
    width2 = radius;
    height2 = radius;

    // Random angular velocity
    angularVel = ((rand() % 100) - 50) / 400.0f;

    // Random initial velocity
    vel.x = ((rand() % 200) - 100) / 80.0f;
    vel.y = ((rand() % 200) - 100) / 80.0f;
}

bool Cuzer::takeHit() {
    if (destroyed) return true;

    hitPoints--;
    if (hitPoints <= 0) {
        destroyed = true;
        return true;
    }
    return false;
}

void Cuzer::initPhysics(PhysicsWorld& world) {
    physicsWorldPtr = &world;
    // Create as dynamic body
    physicsBody = world.createBody(this, PhysicsBodyType::DYNAMIC, 1.0f, 0.3f, 0.8f);
    if (b2Body_IsValid(physicsBody)) {
        b2Body_SetLinearDamping(physicsBody, 0.0f);
        b2Body_SetAngularDamping(physicsBody, 0.0f);
        b2Body_SetGravityScale(physicsBody, 0.0f);  // No gravity for cuzers
        // Set initial velocity and angular velocity
        world.setLinearVelocity(physicsBody, vel.x, vel.y);
        b2Body_SetAngularVelocity(physicsBody, angularVel * 60.0f);
    }
}

void Cuzer::syncFromPhysics() {
    if (!physicsWorldPtr || !b2Body_IsValid(physicsBody)) return;

    b2Vec2 physPos = physicsWorldPtr->getPosition(physicsBody);
    b2Vec2 physVel = physicsWorldPtr->getLinearVelocity(physicsBody);
    b2Rot rot = b2Body_GetRotation(physicsBody);

    pos.x = physPos.x;
    pos.y = physPos.y;
    vel.x = physVel.x;
    vel.y = physVel.y;
    angle = b2Rot_GetAngle(rot);
    angularVel = b2Body_GetAngularVelocity(physicsBody);
}

void Cuzer::update(void) {
    if (destroyed) return;

    if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
        syncFromPhysics();
    } else {
        pos.x += vel.x;
        pos.y += vel.y;
        angle += angularVel;
    }
}

void Cuzer::draw(void) {
    if (destroyed) return;

    computeRect();

    // Calculate damage color shift (redder as HP decreases)
    float damageRatio = (float)hitPoints / (float)maxHitPoints;
    float r = baseColor.r + (1.0f - damageRatio) * (1.0f - baseColor.r);
    float g = baseColor.g * damageRatio;
    float b = baseColor.b * damageRatio;
    GameColor bodyColor = map_rgb_f(r, g, b);
    GameColor outlineColor = map_rgb(220, 220, 220);

    // Rotation values
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Diamond size (surrounds the ellipse)
    float diamondSize = radius * 1.2f;

    // Head (circle) offset - positioned above the diamond
    float headRadius = radius * 0.2f;
    float headOffset = diamondSize + headRadius + 2.0f;  // Distance from center to head

    // Ellipse dimensions (body inside diamond)
    float ellipseRx = radius * 0.5f;
    float ellipseRy = radius * 0.4f;

    // Calculate rotated head position (above the diamond in local coords, rotated)
    // In local coords, head is at (0, -headOffset), then rotate by angle
    // But our angle has 0 pointing right, so "up" in local space is angle - PI/2
    float headLocalX = 0;
    float headLocalY = -headOffset;
    float headX = pos.x + headLocalX * cosA - headLocalY * sinA;
    float headY = pos.y + headLocalX * sinA + headLocalY * cosA;

    // Diamond vertices (in local coords, then rotated)
    // Diamond has 4 points: top, right, bottom, left
    float diamondPoints[4][2] = {
        {0, -diamondSize},       // Top
        {diamondSize, 0},        // Right
        {0, diamondSize},        // Bottom
        {-diamondSize, 0}        // Left
    };

    // Rotate diamond points
    float rotatedDiamond[4][2];
    for (int i = 0; i < 4; i++) {
        float lx = diamondPoints[i][0];
        float ly = diamondPoints[i][1];
        rotatedDiamond[i][0] = pos.x + lx * cosA - ly * sinA;
        rotatedDiamond[i][1] = pos.y + lx * sinA + ly * cosA;
    }

    // Draw filled diamond (body area)
    // Triangle 1: top, right, bottom
    draw_filled_triangle(
        rotatedDiamond[0][0], rotatedDiamond[0][1],
        rotatedDiamond[1][0], rotatedDiamond[1][1],
        rotatedDiamond[2][0], rotatedDiamond[2][1],
        bodyColor
    );
    // Triangle 2: top, bottom, left
    draw_filled_triangle(
        rotatedDiamond[0][0], rotatedDiamond[0][1],
        rotatedDiamond[2][0], rotatedDiamond[2][1],
        rotatedDiamond[3][0], rotatedDiamond[3][1],
        bodyColor
    );

    // Draw filled ellipse at center (body)
    // For a rotated ellipse, we need to draw it manually with triangles
    const int segments = 24;
    for (int i = 0; i < segments; i++) {
        float angle1 = (float)i * 2.0f * M_PI / segments;
        float angle2 = (float)(i + 1) * 2.0f * M_PI / segments;

        // Local ellipse points
        float x1 = ellipseRx * cos(angle1);
        float y1 = ellipseRy * sin(angle1);
        float x2 = ellipseRx * cos(angle2);
        float y2 = ellipseRy * sin(angle2);

        // Rotate and translate
        float rx1 = pos.x + x1 * cosA - y1 * sinA;
        float ry1 = pos.y + x1 * sinA + y1 * cosA;
        float rx2 = pos.x + x2 * cosA - y2 * sinA;
        float ry2 = pos.y + x2 * sinA + y2 * cosA;

        // Draw triangle from center
        GameColor innerColor = map_rgb_f(r * 0.7f, g * 0.7f, b * 0.7f);
        draw_filled_triangle(pos.x, pos.y, rx1, ry1, rx2, ry2, innerColor);
    }

    // Draw diamond outline
    draw_line(rotatedDiamond[0][0], rotatedDiamond[0][1],
              rotatedDiamond[1][0], rotatedDiamond[1][1], outlineColor, 2.0f);
    draw_line(rotatedDiamond[1][0], rotatedDiamond[1][1],
              rotatedDiamond[2][0], rotatedDiamond[2][1], outlineColor, 2.0f);
    draw_line(rotatedDiamond[2][0], rotatedDiamond[2][1],
              rotatedDiamond[3][0], rotatedDiamond[3][1], outlineColor, 2.0f);
    draw_line(rotatedDiamond[3][0], rotatedDiamond[3][1],
              rotatedDiamond[0][0], rotatedDiamond[0][1], outlineColor, 2.0f);

    // Draw filled circle (head) above the diamond
    const int headSegments = 16;
    for (int i = 0; i < headSegments; i++) {
        float a1 = (float)i * 2.0f * M_PI / headSegments;
        float a2 = (float)(i + 1) * 2.0f * M_PI / headSegments;

        float hx1 = headX + headRadius * cos(a1);
        float hy1 = headY + headRadius * sin(a1);
        float hx2 = headX + headRadius * cos(a2);
        float hy2 = headY + headRadius * sin(a2);

        draw_filled_triangle(headX, headY, hx1, hy1, hx2, hy2, bodyColor);
    }

    // Draw head outline
    draw_ellipse(headX, headY, headRadius, headRadius, outlineColor, 1.5f);
}
