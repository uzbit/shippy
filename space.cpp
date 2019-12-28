

#include <iostream>
#include "defines.h"
#include "body.h"
#include "space.h"

using namespace std;

Space::Space(int body_count, int coordx, int coordy)
:body_count(body_count), coordx(coordx), coordy(coordy){

}

void Space::init(void){
    bodies = new Body* [body_count];
    //Body(float x, float y, float width, float height, ALLEGRO_COLOR color, bool filled);
    int max_size = 400;
    int min_size = 50;
    for (int i=0; i < body_count; i++){
        float width = min_size + rand()%max_size;
        float height = min_size + rand()%max_size;
        float posx = rand() % (int)(WINDOW_WIDTH - width/2);
        float posy = rand() % (int)(WINDOW_HEIGHT - height/2);
        if (posx - width/2 < 0) posx += width/2;
        if (posy - height/2 < 0) posy += height/2;
         
        bodies[i] = new Body(
            posx, posy,
            width, height, 
            al_map_rgb(rand()%255, rand()%255, rand()%255), 
            rand()%2 > 0 ? true : false
        );
        bodies[i]->computeRect();

    }

    // make one body ground for lowest coordy
    if (coordy == 0){
        bodies[0]->pos.x = WINDOW_WIDTH/2;
        bodies[0]->pos.y = WINDOW_HEIGHT - 25;
        bodies[0]->width = WINDOW_WIDTH+50;
        bodies[0]->height = 50;
        bodies[0]->width2 = bodies[0]->width/2;
        bodies[0]->height2 = bodies[0]->height/2;
        bodies[0]->filled = true;
        bodies[0]->computeRect();
    }
    
    // for (int i=0; i < body_count; i++)
    //     printf("%f, %f, %f, %f\n", 
    //         bodies[i]->rect.tl.x, bodies[i]->rect.tl.y, bodies[i]->rect.br.x, bodies[i]->rect.br.y);
    
}

void Space::draw(void){
    for (int i=body_count-1; i >=0 ; i--)
        bodies[i]->draw();
}
