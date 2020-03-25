

#include <iostream>
#include "defines.h"
#include "body.h"
#include "space.h"

using namespace std;

Space::Space(int body_count, int coordx, int coordy, int window_w, int window_h, bool gravitate)
:body_count(body_count), coordx(coordx), coordy(coordy), window_width(window_w), window_height(window_h), gravitate_bodies(gravitate){

}

Point Space::getStartPos(float width, float height){
    float posx = rand() % (int)(window_width - width/2);
    float posy = rand() % (int)(window_height - height/2);
    if (posx - width/2 < 0) posx += width/2;
    if (posy - height/2 < 0) posy += height/2;
    return Point(posx, posy);    
}

void Space::init(int difficulty){
    bodies = new Body* [body_count];
    //Body(float x, float y, float width, float height, ALLEGRO_COLOR color, bool filled);
    int max_size = MAX_BODY_SIZE;
    int min_size = MIN_BODY_SIZE;
    float width, height;
    Point pos;
    for (int i=0; i < body_count; i++){
        pos = getStartPos(width, height);
        width = min_size + rand()%max_size;
        height = min_size + rand()%max_size;
         
        bodies[i] = new Body(
            pos.x, pos.y,
            width, height, 
            al_map_rgb(rand()%255, rand()%255, rand()%255), 
            rand()%2 > 0 ? true : false
        );
        bodies[i]->computeRect();

    }

    // make one body ground for lowest coordy
    if (coordy == 0){
        bodies[0]->pos.x = window_width/2;
        bodies[0]->pos.y = window_height - EARTH_HEIGHT/2;
        bodies[0]->width = window_width;
        bodies[0]->height = EARTH_HEIGHT;
        bodies[0]->width2 = bodies[0]->width/2;
        bodies[0]->height2 = bodies[0]->height/2;
        bodies[0]->filled = true;
        bodies[0]->round = 1;
        bodies[0]->computeRect();
    }
    

    Loot loot;
    int loot_count = rand() % ((10 - difficulty) + 1);
    
    for (int i=0; i < loot_count; i++){
        int type = rand() % NUM_LOOT;
        pos = getStartPos(width, height);
        switch(type){
            case FUEL:
                width = 20;
                height = 30;
                loot = Loot(pos.x, pos.y, width, height, al_map_rgb(255, 20, 20), (LootType)type);
                loot.value = 100 + rand() % (int)FUEL_START/2;
                loots.push_back(loot);
                break;
            case BOOST:
                width = 40;
                height = 40;
                loot = Loot(pos.x, pos.y, width, height, al_map_rgb(255, 200, 20), (LootType)type);
                loot.value = rand() % 5 + 2;
                loots.push_back(loot);
                break;
            
        }
    }

    int duder_count = rand() % 4;
    Duder duder;
    for (int i=0; i < duder_count; i++){
        pos = getStartPos(width, height);
        duder = Duder(pos.x, pos.y, rand()%20 + 20, rand()%15 + 15);
        duders.push_back(duder);
    }  
    
    // for (int i=0; i < body_count; i++)
    //     printf("%f, %f, %f, %f\n", 
    //         bodies[i]->rect.tl.x, bodies[i]->rect.tl.y, bodies[i]->rect.br.x, bodies[i]->rect.br.y);
    
}

void Space::draw(void){
    for (int i=body_count-1; i >=0 ; i--)
        bodies[i]->draw();
    
    for (int i=0; i < loots.size() ; i++)
        loots[i].draw();

    for (int i=0; i < duders.size() ; i++)
        duders[i].draw();
}
