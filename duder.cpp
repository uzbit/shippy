
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <iostream>

#include "duder.h"
#include "geom.h"
#include "defines.h"

using namespace std;

Duder::Duder(float x, float y, int bias_index)
:Object(x, y, 40, 40), bias_index(bias_index){
    pos.x = x;
    pos.y = y;
    vel.x = ((rand() % 100) - 50)/100.0;
    vel.y = ((rand() % 100) - 50)/100.0;
}


void Duder::update(void){
    pos.x += vel.x;
    pos.y += vel.y;
    vel.x += accel.x;
    vel.y += GRAVITY + accel.y;
    accel.x = 0;
    accel.y = 0;
}

void Duder::draw_flame(ALLEGRO_TRANSFORM *transform){
    al_use_transform(transform);
    al_draw_filled_triangle(-15, 0, 0, width2, 0, -width2, al_map_rgb(25, 200, 2));
    al_identity_transform(transform);
    al_use_transform(transform); 
}

void Duder::draw_flames(void){
    if (thrust_dir != NONE){
        ALLEGRO_TRANSFORM transform;
        if (thrust_dir & LEFT){
            al_build_transform(&transform, pos.x+width2+thick/2, pos.y, 0.75, 0.75, -3.14159);
            draw_flame(&transform); 
            flame_counter[0]++;
        }
        if (thrust_dir & RIGHT){
            al_build_transform(&transform, pos.x-width2-thick/2, pos.y, 0.75, 0.75, 0);
            draw_flame(&transform);
            flame_counter[1]++;    
        }               
        if (thrust_dir & UP){
            al_build_transform(&transform, pos.x, pos.y+height2+thick/2, 1, 1, -3.14159/2);
            draw_flame(&transform);
            flame_counter[2]++; 
        }
        if (thrust_dir & DOWN){
            al_build_transform(&transform, pos.x, pos.y-height2-thick/2, 1, 1, 3.14159/2);
            draw_flame(&transform);
            flame_counter[3]++; 
        }
    }
}

void Duder::draw(void){
    computeRect();
    
    draw_flames();
    
    al_draw_rounded_rectangle(
        rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
        1, 1, al_map_rgb(255, 255, 2), thick
    );
    
    if (fuel > 0){
        float fuel_ratio = (fuel_start - fuel)/fuel_start;
        float tly = (rect.br.y-thick/2) - ((rect.br.y-thick/2) - (rect.tl.y+thick/2))*(1-fuel_ratio);
        al_draw_filled_rectangle(
            rect.tl.x+thick/2, tly, rect.br.x-thick/2, rect.br.y-thick/2,
            al_map_rgb(255, 30, 2)
        );
    }
    
    if (flame_counter[0] >= 3)
        thrust_dir &= !LEFT;
    if (flame_counter[1] >= 3)
        thrust_dir &= !RIGHT;
    if (flame_counter[2] >= 3)
        thrust_dir &= !UP;
    if (flame_counter[3] >= 3)
        thrust_dir &= !DOWN;
    
}
