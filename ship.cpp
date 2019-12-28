
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <iostream>

using namespace std;

#include "ship.h"
#include "geom.h"
#include "defines.h"


Ship::Ship(){
}
Ship::~Ship(){}

Ship::Ship(float x, float y, float fuel, float mass)
:Object(x, y, 10, 40), fuel(fuel), mass(mass){
    pos.x=x;
    pos.y=y;
    thrustx = 2500.0;
    thrusty = 10000.0;
    fuel_start = fuel;
}

void Ship::thrust_horizontal(float scale){
    if (fuel > 0){
        accel.x = scale*thrustx/(mass + fuel*FUEL_MASS);
        fuel -= HORIZONTAL_FUEL_CONSUMPTION;
        if (scale < 0) thrust_dir = LEFT;
        if (scale > 0) thrust_dir = RIGHT;
        
    }
    if (fuel < 0){
        fuel = 0;
        thrust_dir = NONE;
    }
    cout << "ACCELX " << accel.x << endl;
}
void Ship::thrust_vertical(float scale){
    if (fuel > 0){
        accel.y = scale*thrusty/(mass + fuel*FUEL_MASS);
        fuel -= VERTICAL_FUEL_CONSUMPTION;
        if (scale < 0) thrust_dir = UP;
        if (scale > 0) thrust_dir = DOWN;
    }
    if (fuel < 0){
        fuel = 0;
        thrust_dir = NONE;
    } 
    
    cout << "ACCELY " << accel.y << endl;
}

void Ship::update(void){
    pos.x += vel.x;
    pos.y += vel.y;
    vel.x += accel.x;
    vel.y += GRAVITY + accel.y;
    accel.x = 0;
    accel.y = 0;
}

void Ship::draw(void){

    float thick = 4;
    
    computeRect();
    
    ALLEGRO_TRANSFORM transform;
    switch(thrust_dir){
        case RIGHT:
            al_build_transform(&transform, pos.x-width2-thick/2, pos.y, 1, 1, 0);
            break;
        case LEFT:
            al_build_transform(&transform, pos.x+width2+thick/2, pos.y, 1, 1, -3.14159);
            break;
        case UP:
            al_build_transform(&transform, pos.x, pos.y+height2+thick/2, 1, 1, -3.14159/2);
            break;
        case DOWN:
            al_build_transform(&transform, pos.x, pos.y-height2-thick/2, 1, 1, 3.14159/2);
            break;
        default:
            al_identity_transform(&transform);
    }
    al_use_transform(&transform);
    al_draw_filled_triangle(-15, 0, 0, 10, 0, -10, al_map_rgb(25, 200, 2));
    al_identity_transform(&transform);
    al_use_transform(&transform);
    
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
    thrust_dir = NONE;
    
}
