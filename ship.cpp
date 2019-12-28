
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <iostream>

#include "ship.h"
#include "geom.h"
#include "defines.h"

using namespace std;

Ship::Ship(){
}
Ship::~Ship(){}

Ship::Ship(float x, float y, float fuel, float mass)
:Object(x, y, 10, 40), fuel(fuel), mass(mass), thick(4){
    pos.x=x;
    pos.y=y;
    thrustx = 250.0;
    thrusty = 1000.0;
    fuel_start = fuel;
    thrust_dir = NONE;
    memset(flame_counter, 0, sizeof(flame_counter));
}

void Ship::thrust_horizontal(float scale){
    if (fuel > 0){
        accel.x = scale*thrustx/(mass + fuel*FUEL_MASS);
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
    cout << "ACCELX " << accel.x << endl;
}
void Ship::thrust_vertical(float scale){
    if (fuel > 0){
        accel.y = scale*thrusty/(mass + fuel*FUEL_MASS);
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

void Ship::draw_flame(ALLEGRO_TRANSFORM *transform){
    al_use_transform(transform);
    al_draw_filled_triangle(-15, 0, 0, width2, 0, -width2, al_map_rgb(25, 200, 2));
    al_identity_transform(transform);
    al_use_transform(transform); 
}

void Ship::draw_flames(void){
    
    if (thrust_dir != NONE){
        ALLEGRO_TRANSFORM transform;
        if (thrust_dir & LEFT){
            al_build_transform(&transform, pos.x+width2+thick/2, pos.y, 1, 1, -3.14159);
            draw_flame(&transform); 
            flame_counter[0]++;
        }
        if (thrust_dir & RIGHT){
            al_build_transform(&transform, pos.x-width2-thick/2, pos.y, 1, 1, 0);
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

void Ship::draw(void){

    
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
