
#include <stdio.h>
#include "sdl_compat.h"
#include "object.h"
#include "body.h"
#include "physics_world.h"


Body::Body(float x, float y, float width, float height, GameColor color, bool filled)
:Object(x, y, width, height), color(color), filled(filled){
    round = rand() % 20 + 1;
    density = (100 + rand() % 100) / 100.0;
    thick = rand() % 8 + 2;
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
    } else if (!filled){
        draw_rounded_rect(
            rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
            round, round, color, thick
        );
    } else {
        draw_filled_rounded_rect(
            rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
            round, round, color
        );
    }
}
