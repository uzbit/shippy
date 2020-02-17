
#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <iostream>

#include "starfield.h"
#include "geom.h"
#include "defines.h"

using namespace std;


Star::Star(float x, float y, int ls)
:Object(x, y, 1, 1){
    lifespan = ls;
    color = al_map_rgb(rand()%255, rand()%255, rand()%255);
    
    counter = rand() % (lifespan);
}

void Star::update(){
    if (counter > lifespan){
        counter = 0;
        pos.x = rand() % int(WINDOW_WIDTH);
        pos.y = rand() % int(WINDOW_HEIGHT);
    }
    counter++;
}

void Star::draw(){
    float scaledCounter = float(counter) / (lifespan);
    float scale = 4*(scaledCounter - scaledCounter*scaledCounter);
    al_draw_filled_ellipse(
        pos.x, pos.y, width, height,
        al_map_rgb_f(
            scale,
            scale,
            scale
        )
    ); 
}

Starfield::Starfield(){
    num_stars = 75 + rand() % 50;
    int posx, posy, lifespan, delay;
    for(int i = 0; i < num_stars; i++){
        posx = rand() % int(WINDOW_WIDTH);
        posy = rand() % int(WINDOW_HEIGHT);
        //cout << posx << posy << endl;
        lifespan = 75 + rand() % 500;
        stars.push_back(Star(
            posx, posy, lifespan
        ));
    }
}


void Starfield::update(void){
    for(int i = 0; i < stars.size(); i++){
        stars[i].update();
    }
}

void Starfield::draw(void){
    for(int i = 0; i < stars.size(); i++){
        stars[i].draw();
    }
}
