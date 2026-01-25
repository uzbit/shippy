
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
}

void Body::initPhysics(PhysicsWorld& world) {
    physicsBody = world.createBody(this, PhysicsBodyType::STATIC, 0.0f, 0.3f, 0.1f);
}

void Body::draw(void){
    if (!filled){
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

