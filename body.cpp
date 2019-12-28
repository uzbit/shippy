
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include "object.h"
#include "body.h"


Body::Body(float x, float y, float width, float height, ALLEGRO_COLOR color, bool filled)
:Object(x, y, width, height), color(color), filled(filled){
    round = rand() % 6 + 1; 
}

void Body::draw(void){
    if (!filled){
        int thick = 2; //rand() % 8 + 2;
        al_draw_rounded_rectangle(
            rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
            round, round, color, thick
        );
    } else {
        al_draw_filled_rounded_rectangle(
            rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
            round, round, color
        );
    }
}


