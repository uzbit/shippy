
#include <stdio.h>
#include "sdl_compat.h"
#include "object.h"
#include "body.h"
#include "physics_world.h"


Body::Body(float x, float y, float width, float height, GameColor color)
:Object(x, y, width, height), color(color){
    round = rand() % 20 + 1;
    density = (100 + rand() % 100) / 100.0;
    gravityStrength = 0.0f;
    isGravityWell = false;
}

Body::~Body() {
}

void Body::initPhysics(PhysicsWorld& world) {
    physicsBody = world.createBody(this, PhysicsBodyType::STATIC, 0.0f, 0.3f, 0.1f);
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
        draw_filled_ellipse(cx, cy, rx * 1.15f, ry * 1.15f, atmo);

        // Main body
        draw_filled_ellipse(cx, cy, rx, ry, color);

        // Surface bands
        GameColor band = {color.r * 0.7f, color.g * 0.7f, color.b * 0.7f, 0.3f};
        int num_bands = 2 + (round % 4);
        for (int i = 0; i < num_bands; i++) {
            float band_y = cy - ry + (i + 1) * (ry * 2.0f) / (num_bands + 1);
            float band_dist = fabsf(band_y - cy) / ry;
            float band_rx = rx * sqrtf(std::max(0.0f, 1.0f - band_dist * band_dist));
            if (band_rx > 2.0f)
                draw_line(cx - band_rx * 0.8f, band_y, cx + band_rx * 0.8f, band_y, band, 1.5f);
        }

        // Highlight
        GameColor highlight = {1.0f, 1.0f, 1.0f, 0.1f};
        draw_filled_ellipse(cx - rx * 0.25f, cy - ry * 0.25f,
                            rx * 0.5f, ry * 0.5f, highlight);

        // Outline
        GameColor outline = {color.r * 0.5f, color.g * 0.5f, color.b * 0.5f, 0.6f};
        draw_ellipse(cx, cy, rx, ry, outline, 1.5f);
    }
}
