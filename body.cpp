
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include "object.h"
#include "body.h"


Body::Body(float x, float y, float width, float height, ALLEGRO_COLOR color, bool filled)
:Object(x, y, width, height), color(color), filled(filled){
    round = rand() % 20 + 1; 
    density = (100 + rand() % 100) / 100.0;
    thick = rand() % 8 + 2;
}

void Body::draw(void){
    if (!filled){
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


