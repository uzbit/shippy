#include <SDL3/SDL.h>
#include <math.h>
#include <stdlib.h>

#include "asteroid.h"
#include "physics_world.h"
#include "defines.h"

Asteroid::Asteroid()
    : Object(0, 0, 40, 40), angle(0), angularVel(0), destroyed(false),
      size(AsteroidSize::MEDIUM), radius(20), color(map_rgb(150, 150, 150)) {
    numVertices = 8;
    for (int i = 0; i < numVertices; i++) {
        vertices[i] = 0.8f + (rand() % 40) / 100.0f;  // 0.8 to 1.2 variation
    }
}

Asteroid::Asteroid(float x, float y, AsteroidSize sz)
    : Object(x, y, 40, 40), angle(0), destroyed(false), size(sz),
      color(map_rgb(120 + rand() % 60, 100 + rand() % 50, 80 + rand() % 40)) {

    // Set radius based on size
    switch (size) {
        case AsteroidSize::SMALL:
            radius = 10 + rand() % 10;  // 10-20
            break;
        case AsteroidSize::MEDIUM:
            radius = 25 + rand() % 15;  // 25-40
            break;
        case AsteroidSize::LARGE:
            radius = 50 + rand() % 30;  // 50-80
            break;
    }

    width = radius * 2;
    height = radius * 2;
    width2 = radius;
    height2 = radius;

    // Random angular velocity
    angularVel = ((rand() % 100) - 50) / 500.0f;

    // Random initial velocity
    vel.x = ((rand() % 200) - 100) / 50.0f;
    vel.y = ((rand() % 200) - 100) / 50.0f;

    // Generate irregular shape vertices
    numVertices = 6 + rand() % 4;  // 6-9 vertices
    for (int i = 0; i < numVertices; i++) {
        vertices[i] = 0.7f + (rand() % 50) / 100.0f;  // 0.7 to 1.2 variation
    }
}

void Asteroid::initPhysics(PhysicsWorld& world) {
    physicsWorldPtr = &world;
    // Create as dynamic body
    physicsBody = world.createBody(this, PhysicsBodyType::DYNAMIC, 1.0f, 0.3f, 0.8f);
    if (b2Body_IsValid(physicsBody)) {
        b2Body_SetLinearDamping(physicsBody, 0.0f);
        b2Body_SetAngularDamping(physicsBody, 0.0f);
        b2Body_SetGravityScale(physicsBody, 0.0f);  // No gravity for asteroids
        // Set initial velocity and angular velocity
        world.setLinearVelocity(physicsBody, vel.x, vel.y);
        b2Body_SetAngularVelocity(physicsBody, angularVel * 60.0f);
    }
}

void Asteroid::syncFromPhysics() {
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

void Asteroid::update(void) {
    if (destroyed) return;

    if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
        syncFromPhysics();
    } else {
        pos.x += vel.x;
        pos.y += vel.y;
        angle += angularVel;
    }
}

void Asteroid::draw(void) {
    if (destroyed) return;

    // Draw asteroid as an irregular polygon
    float cosA = cos(angle);
    float sinA = sin(angle);

    // Draw filled polygon using triangles
    for (int i = 0; i < numVertices; i++) {
        int next = (i + 1) % numVertices;

        float angle1 = (float)i * 2.0f * M_PI / numVertices;
        float angle2 = (float)next * 2.0f * M_PI / numVertices;

        float r1 = radius * vertices[i];
        float r2 = radius * vertices[next];

        // Local coordinates
        float x1 = r1 * cos(angle1);
        float y1 = r1 * sin(angle1);
        float x2 = r2 * cos(angle2);
        float y2 = r2 * sin(angle2);

        // Rotate
        float rx1 = x1 * cosA - y1 * sinA;
        float ry1 = x1 * sinA + y1 * cosA;
        float rx2 = x2 * cosA - y2 * sinA;
        float ry2 = x2 * sinA + y2 * cosA;

        // Translate
        rx1 += pos.x; ry1 += pos.y;
        rx2 += pos.x; ry2 += pos.y;

        // Draw triangle from center to edge
        draw_filled_triangle(pos.x, pos.y, rx1, ry1, rx2, ry2, color);
    }

    // Draw outline for better visibility
    GameColor outlineColor = map_rgb(200, 200, 200);
    for (int i = 0; i < numVertices; i++) {
        int next = (i + 1) % numVertices;

        float angle1 = (float)i * 2.0f * M_PI / numVertices;
        float angle2 = (float)next * 2.0f * M_PI / numVertices;

        float r1 = radius * vertices[i];
        float r2 = radius * vertices[next];

        float x1 = r1 * cos(angle1);
        float y1 = r1 * sin(angle1);
        float x2 = r2 * cos(angle2);
        float y2 = r2 * sin(angle2);

        float rx1 = x1 * cosA - y1 * sinA + pos.x;
        float ry1 = x1 * sinA + y1 * cosA + pos.y;
        float rx2 = x2 * cosA - y2 * sinA + pos.x;
        float ry2 = x2 * sinA + y2 * cosA + pos.y;

        draw_line(rx1, ry1, rx2, ry2, outlineColor, 2.0f);
    }
}
