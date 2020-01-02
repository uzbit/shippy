
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <math.h>
#include "object.h"
#include "loot.h"
#include "geom.h"


Loot::Loot(float x, float y, float width, float height, ALLEGRO_COLOR color, LootType type)
:Object(x, y, width, height), color(color), type(type){
}

void applyRot(Point *p, float theta){
    float x = p->x*cos(theta) - p->y*sin(theta);
    float y = p->x*sin(theta) + p->y*cos(theta);
    p->x = x;
    p->y = y;
}

void Loot::draw(void){
    switch(type){
        case FUEL:
            al_draw_rounded_rectangle(
                rect.tl.x, rect.tl.y, rect.br.x, rect.br.y,
                4, 4, color, 4
            );
            al_draw_line(rect.tl.x, rect.tl.y, rect.br.x, rect.br.y, color, 2);
            al_draw_line(rect.br.x, rect.tl.y, rect.tl.x, rect.br.y, color, 2);
            al_draw_line(rect.tl.x, rect.tl.y, rect.tl.x-4, rect.tl.y-4, color, 3);
            al_draw_line(rect.tl.x-4, rect.tl.y-4, rect.tl.x-10, rect.tl.y-4, color, 3); 
            break;
        case BOOST:
            Point p1, p2, p3;
            float d = 0.5*width2;
            float deg2rad = M_PI/180;
            float a = d*cos(54*deg2rad);
            float b = d*sin(54*deg2rad);
            float c, g, theta = 2*M_PI/5.0;
            
            p1.x = 0; 
            p1.y = -height2;
            p2.x =  a;
            p2.y = -b;

            c = sqrt((p1.x-p2.x)*(p1.x-p2.x) + (p1.y-p2.y)*(p1.y-p2.y));
            g = c + a;
            
            p3.x =  g;
            p3.y = -g*tan(18*deg2rad);
            
            for (int i=0; i < 5; i++) {
                applyRot(&p1, theta);
                applyRot(&p2, theta);
                applyRot(&p3, theta);
                al_draw_line(pos.x + p1.x, pos.y + p1.y, 
                             pos.x + p2.x, pos.y + p2.y, color, 4);
                al_draw_line(pos.x + p2.x, pos.y + p2.y, 
                             pos.x + p3.x, pos.y + p3.y, color, 4);
            }
            break;
    } 
}


