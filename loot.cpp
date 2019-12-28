
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include "object.h"
#include "loot.h"


Loot::Loot(float x, float y, float width, float height, ALLEGRO_COLOR color, LootType type)
:Object(x, y, width, height), color(color), type(type){
}

void Loot::draw(void){
    if (type == FUEL){
        al_draw_rounded_rectangle(
            rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
            4, 4, color, 4
        );
        al_draw_line(rect.tl.x, rect.tl.y, rect.br.x, rect.br.y, color, 2);
        al_draw_line(rect.br.x, rect.tl.y, rect.tl.x, rect.br.y, color, 2);
        al_draw_line(rect.tl.x, rect.tl.y, rect.tl.x-4, rect.tl.y-4, color, 3);
        al_draw_line(rect.tl.x-4, rect.tl.y-4, rect.tl.x-10, rect.tl.y-4, color, 3);  
    } 
}


