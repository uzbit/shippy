
#include <stdio.h>
#include <cmath>
#include "sdl_compat.h"
#include "object.h"
#include "body.h"
#include "physics_world.h"

Body::Body(float x, float y, float width, float height, GameColor color)
:Object(x, y, width, height), color(color){
    density = (100 + rand() % 100) / 100.0;
    gravityStrength = 0.0f;
    isGravityWell = false;

    // Random rotation
    rotation = (rand() % 3600) / 3600.0f * 2.0f * M_PI;

    // Generate jagged edge — each vertex gets a unique radius multiplier
    for (int i = 0; i < NUM_EDGE_VERTS; i++) {
        float jitter = (rand() % 100) / 100.0f;
        edgeRadii[i] = 0.85f + jitter * 0.2f;
    }
    // Smooth the edge
    float smoothed[NUM_EDGE_VERTS];
    for (int i = 0; i < NUM_EDGE_VERTS; i++) {
        int prev = (i + NUM_EDGE_VERTS - 1) % NUM_EDGE_VERTS;
        int next = (i + 1) % NUM_EDGE_VERTS;
        smoothed[i] = edgeRadii[prev] * 0.25f + edgeRadii[i] * 0.5f + edgeRadii[next] * 0.25f;
    }
    for (int i = 0; i < NUM_EDGE_VERTS; i++) edgeRadii[i] = smoothed[i];

    // Generate craters — unique per body
    float avg_r = (width + height) / 4.0f;
    int num_craters = 2 + rand() % 5;
    for (int i = 0; i < num_craters; i++) {
        Crater c;
        c.angle = (rand() % 3600) / 3600.0f * 2.0f * M_PI;
        c.dist = 0.15f + (rand() % 60) / 100.0f;
        c.radius = avg_r * (0.06f + (rand() % 12) / 100.0f);
        craters.push_back(c);
    }
}

Body::~Body() {
}

void Body::initPhysics(PhysicsWorld& world) {
    float avg_radius = (width2 + height2) / 2.0f;
    physicsBody = world.createCircleBody(this, PhysicsBodyType::STATIC, avg_radius, 0.0f, 0.3f, 0.1f);
}

void Body::draw(void){
    if (isGravityWell) {
        float cx = pos.x;
        float cy = pos.y;
        float maxRadius = (width2 + height2) / 2;

        // Accretion disk glow
        GameColor diskGlow = {color.r * 0.3f, color.g * 0.2f, color.b * 0.5f, 0.06f};
        draw_filled_ellipse(cx, cy, maxRadius * 1.5f, maxRadius * 1.5f, diskGlow);

        for (int i = 5; i >= 1; i--) {
            float radius = maxRadius * (i / 5.0f);
            float t = i / 5.0f;
            GameColor ringColor = map_rgb(
                (int)(100 + 80 * t),
                (int)(20 + 30 * t),
                (int)(180 + 75 * t)
            );
            draw_ellipse(cx, cy, radius, radius, ringColor, 2.0f + i);
        }

        GameColor coreColor = map_rgb(50, 0, 100);
        draw_filled_ellipse(cx, cy, maxRadius * 0.2f, maxRadius * 0.2f, coreColor);
    } else {
        float cx = pos.x;
        float cy = pos.y;
        float rx = width2;
        float ry = height2;

        // Atmosphere glow
        GameColor atmo = {color.r, color.g, color.b, 0.08f};
        draw_filled_ellipse(cx, cy, rx * 1.2f, ry * 1.2f, atmo);

        // Draw jagged body as filled triangle fan (rotated)
        float cosR = cosf(rotation), sinR = sinf(rotation);
        for (int i = 0; i < NUM_EDGE_VERTS; i++) {
            int next = (i + 1) % NUM_EDGE_VERTS;
            float a1 = i * 2.0f * M_PI / NUM_EDGE_VERTS;
            float a2 = next * 2.0f * M_PI / NUM_EDGE_VERTS;

            // Local coords
            float lx1 = cosf(a1) * rx * edgeRadii[i];
            float ly1 = sinf(a1) * ry * edgeRadii[i];
            float lx2 = cosf(a2) * rx * edgeRadii[next];
            float ly2 = sinf(a2) * ry * edgeRadii[next];

            // Rotate
            float x1 = cx + lx1 * cosR - ly1 * sinR;
            float y1 = cy + lx1 * sinR + ly1 * cosR;
            float x2 = cx + lx2 * cosR - ly2 * sinR;
            float y2 = cy + lx2 * sinR + ly2 * cosR;

            draw_filled_triangle(cx, cy, x1, y1, x2, y2, color);
        }

        // Draw craters (rotated with the body)
        GameColor craterColor = {color.r * 0.4f, color.g * 0.4f, color.b * 0.4f, 0.5f};
        GameColor craterRim = {color.r * 0.8f, color.g * 0.8f, color.b * 0.8f, 0.3f};
        for (auto& c : craters) {
            float lx = cosf(c.angle) * rx * c.dist;
            float ly = sinf(c.angle) * ry * c.dist;
            float cr_x = cx + lx * cosR - ly * sinR;
            float cr_y = cy + lx * sinR + ly * cosR;
            draw_filled_ellipse(cr_x, cr_y, c.radius * 1.2f, c.radius * 1.2f, craterRim);
            draw_filled_ellipse(cr_x, cr_y, c.radius, c.radius, craterColor);
        }

        // Highlight (light from upper-left, doesn't rotate)
        GameColor highlight = {1.0f, 1.0f, 1.0f, 0.08f};
        draw_filled_ellipse(cx - rx * 0.2f, cy - ry * 0.2f,
                            rx * 0.45f, ry * 0.45f, highlight);

        // Jagged outline (rotated)
        GameColor outline = {color.r * 0.4f, color.g * 0.4f, color.b * 0.4f, 0.7f};
        for (int i = 0; i < NUM_EDGE_VERTS; i++) {
            int next = (i + 1) % NUM_EDGE_VERTS;
            float a1 = i * 2.0f * M_PI / NUM_EDGE_VERTS;
            float a2 = next * 2.0f * M_PI / NUM_EDGE_VERTS;

            float lx1 = cosf(a1) * rx * edgeRadii[i];
            float ly1 = sinf(a1) * ry * edgeRadii[i];
            float lx2 = cosf(a2) * rx * edgeRadii[next];
            float ly2 = sinf(a2) * ry * edgeRadii[next];

            float x1 = cx + lx1 * cosR - ly1 * sinR;
            float y1 = cy + lx1 * sinR + ly1 * cosR;
            float x2 = cx + lx2 * cosR - ly2 * sinR;
            float y2 = cy + lx2 * sinR + ly2 * cosR;

            draw_line(x1, y1, x2, y2, outline, 1.5f);
        }
    }
}
