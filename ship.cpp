
#include <SDL3/SDL.h>
#include <iostream>
#include <math.h>

#include "ship.h"
#include "space.h"
#include "body.h"
#include "geom.h"
#include "defines.h"
#include "sdl_compat.h"

using namespace std;

Ship::Ship(float x, float y, float fuel, float mass)
:Object(x, y, 10, 40), fuel(fuel), mass(mass), thick(4){
    pos.x = x;
    pos.y = y;
    prev_pos.x = x;
    prev_pos.y = y;
    thrustx = THRUST_X;
    thrusty = THRUST_Y;
    offset = SDL_GetTicks() / 1000.0f;
    fuel_start = fuel;
    thrust_dir = NONE;
    memset(flame_counter, 0, sizeof(flame_counter));
}

void Ship::thrust_horizontal(float scale){
    if (fuel > 0){
        accel.x += scale*thrustx/(mass + fuel*FUEL_MASS);
        fuel -= HORIZONTAL_FUEL_CONSUMPTION;
        if (scale < 0){
            thrust_dir |= LEFT;
            flame_counter[0] = 0;
        }
        if (scale > 0){
            thrust_dir |= RIGHT;
            flame_counter[1] = 0;
        }
    }
    if (fuel < 0){
        fuel = 0;
        thrust_dir = NONE;
    }
    //cout << "ACCELX " << accel.x << endl;
}

void Ship::thrust_vertical(float scale){
    if (fuel > 0){
        accel.y += scale*thrusty/(mass + fuel*FUEL_MASS);
        fuel -= VERTICAL_FUEL_CONSUMPTION;
        if (scale < 0){
            thrust_dir |= UP;
            flame_counter[2] = 0;
        }
        if (scale > 0){
            thrust_dir |= DOWN;
            flame_counter[3] = 0;
        }
    }
    if (fuel < 0){
        fuel = 0;
        thrust_dir = NONE;
    }
    //cout << "ACCELY " << accel.y << endl;
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
    pos.x += vel.x;
    pos.y += vel.y;
    vel.x += accel.x;
    vel.y += 0.01 + accel.y; //gravity = 0.01
    accel.x = 0;
    accel.y = 0;
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

void Ship::draw_flames(void){
    if (thrust_dir != NONE){
        if (thrust_dir & LEFT){
            draw_flame(pos.x+width2+thick/2, pos.y, 0.75, -M_PI);
            flame_counter[0]++;
        }
        if (thrust_dir & RIGHT){
            draw_flame(pos.x-width2-thick/2, pos.y, 0.75, 0);
            flame_counter[1]++;
        }
        if (thrust_dir & UP){
            draw_flame(pos.x, pos.y+height2+thick/2, 1.0, -M_PI/2);
            flame_counter[2]++;
        }
        if (thrust_dir & DOWN){
            draw_flame(pos.x, pos.y-height2-thick/2, 1.0, M_PI/2);
            flame_counter[3]++;
        }
    }
}

void Ship::draw(void){
    computeRect();

    draw_flames();

    // Draw ship body (rounded rectangle outline)
    GameColor ship_color = map_rgb(2, 255, 255);
    draw_rounded_rect(
        rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
        1, 1, ship_color, thick
    );

    // Draw fuel gauge
    if (fuel > 0){
        float fuel_ratio = (fuel_start - fuel)/fuel_start;
        float tly = (rect.br.y-thick/2) - ((rect.br.y-thick/2) - (rect.tl.y+thick/2))*(1-fuel_ratio);
        GameColor fuel_color = map_rgb(255, 30, 2);
        draw_filled_rect(
            rect.tl.x+thick/2, tly, rect.br.x-thick/2, rect.br.y-thick/2,
            fuel_color
        );
    }

    if (flame_counter[0] >= 3)
        thrust_dir &= ~LEFT;
    if (flame_counter[1] >= 3)
        thrust_dir &= ~RIGHT;
    if (flame_counter[2] >= 3)
        thrust_dir &= ~UP;
    if (flame_counter[3] >= 3)
        thrust_dir &= ~DOWN;

}
