
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>

#include "ship.h"
#include "geom.h"

Ship::Ship(){

}

Ship::Ship(int x, int y, float fuel, float mass)
:fuel(fuel), mass(mass){
    center.x=x;
    center.y=y;
}

void Ship::draw(void){



}
