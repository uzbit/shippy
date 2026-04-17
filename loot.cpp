
#include <stdio.h>
#include <math.h>
#include "sdl_compat.h"
#include "object.h"
#include "loot.h"
#include "geom.h"
#include "physics_world.h"


Loot::Loot(float x, float y, float width, float height, GameColor color, LootType type)
:Object(x, y, width, height), color(color), type(type){
}

void Loot::initPhysics(PhysicsWorld& world) {
    physicsBody = world.createBody(this, PhysicsBodyType::SENSOR, 0.0f, 0.0f, 0.0f);
}

void applyRot(Point *p, float theta){
    float x = p->x*cos(theta) - p->y*sin(theta);
    float y = p->x*sin(theta) + p->y*cos(theta);
    p->x = x;
    p->y = y;
}

void Loot::draw(void){
    switch(type){
        case FUEL: {
            float x1 = pos.x - width2, y1 = pos.y - height2;
            float x2 = pos.x + width2, y2 = pos.y + height2;
            draw_rounded_rect(x1, y1, x2, y2, 4, 4, color, 4);
            draw_line(x1, y1, x2, y2, color, 2);
            draw_line(x2, y1, x1, y2, color, 2);
            draw_line(x1, y1, x1-4, y1-4, color, 3);
            draw_line(x1-4, y1-4, x1-10, y1-4, color, 3);
            break;
        }
        case BOOST: {
            Point p1, p2, p3;
            float d = 0.5*width2;
            float deg2rad = M_PI/180;
            float a = d*cos(54*deg2rad);
            float b = d*sin(54*deg2rad);
            float c, g, theta = 2*M_PI/5.0;

            p1.x = 0;
            p1.y = -height2;
            p2.x =  a;
            p2.y = -b;

            c = sqrt((p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y));
            g = c + a;

            p3.x =  g;
            p3.y = -g*tan(18*deg2rad);

            for (int i=0; i < 5; i++) {
                applyRot(&p1, theta);
                applyRot(&p2, theta);
                applyRot(&p3, theta);
                draw_line(pos.x + p1.x, pos.y + p1.y,
                         pos.x + p2.x, pos.y + p2.y, color, 4);
                draw_line(pos.x + p2.x, pos.y + p2.y,
                         pos.x + p3.x, pos.y + p3.y, color, 4);
            }
            break;
        }
        case MUSHROOM: {
            // Stem
            float stem_w = width2 * 0.4f;
            float stem_h = height2 * 0.8f;
            draw_filled_rounded_rect(
                pos.x - stem_w, pos.y,
                pos.x + stem_w, pos.y + stem_h,
                3, 3, map_rgb(220, 200, 180)
            );
            // Cap (dome on top)
            draw_filled_ellipse(pos.x, pos.y, width2, height2 * 0.7f, color);
            // Spots on cap
            float spot_r = width2 * 0.15f;
            draw_filled_ellipse(pos.x - width2 * 0.35f, pos.y - height2 * 0.15f, spot_r, spot_r, map_rgb(255, 255, 255));
            draw_filled_ellipse(pos.x + width2 * 0.3f, pos.y - height2 * 0.25f, spot_r * 0.8f, spot_r * 0.8f, map_rgb(255, 255, 255));
            draw_filled_ellipse(pos.x + width2 * 0.05f, pos.y - height2 * 0.4f, spot_r * 0.7f, spot_r * 0.7f, map_rgb(255, 255, 255));
            break;
        }
        case NUM_LOOT:
            break;
    }
}

