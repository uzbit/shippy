
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
    }
    if (fuel < 0) fuel = 0;
    cout << "ACCELX " << accel.x << endl;
}
void Ship::thrust_vertical(float scale){
    if (fuel > 0){
        accel.y = scale*thrusty/(mass + fuel*FUEL_MASS);
        fuel -= VERTICAL_FUEL_CONSUMPTION;
    }
    if (fuel < 0) fuel = 0;
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

    //al_draw_line(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, , 10);
    //cout << pos.x << " " << pos.y << endl;
    computeRect();
    
    float thick = 4;
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

}
