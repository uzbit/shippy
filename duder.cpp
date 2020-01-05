
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <iostream>

#include "duder.h"
#include "geom.h"
#include "defines.h"

using namespace std;

Duder::Duder(float x, float y, float width, float height)
:Object(x, y, width, height){
    pos.x = x;
    pos.y = y;
    vel.x = ((rand() % 200) - 100)/50.0;
    vel.y = ((rand() % 200) - 100)/50.0;
    color = al_map_rgb(rand()%255, rand()%255, rand()%255);
    random_val = rand();
    thick = rand() % 10  +1;
    is_killed = false;
}


void Duder::update(void){
    if (!is_killed){
        pos.x += vel.x;
        pos.y += vel.y;
        if (pos.x >= WINDOW_WIDTH || pos.x <= 0) vel.x = -vel.x;
        if (pos.y >= WINDOW_HEIGHT || pos.y <= 0) vel.y = -vel.y;
    }
}


void Duder::draw(void){
    computeRect();
    
    if (!is_killed){
        al_draw_ellipse(
            pos.x, pos.y-height2, width2, height2,
            color, thick
        );
        al_draw_filled_ellipse(
            pos.x, pos.y, width, height2,
            color
        );
    }  
}
