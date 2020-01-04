#include <allegro5/allegro5.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "defines.h"
#include "game.h"
#include "space.h"
#include "ship.h"
#include "loot.h"
#include "body.h"

using namespace std;

Game::Game()
:coordx(0), coordy(0), space_index(-1), done(false){
}

Game::~Game(){
    delete ship;
    for (int i = 0; i < spaces.size(); i++){
        for (int j=0; j < spaces[i].body_count; j++){
            delete spaces[i].bodies[j];
        }
        delete spaces[i].bodies;
    }
    cout << "cleaned up" << endl;
}

void Game::init_graphics(void){
    if (!al_init())
        abort("Failed to initialize allegro");
 
    if (!al_install_keyboard())
        abort("Failed to install keyboard");
    
    timer = al_create_timer(1.0 / 60);
    if (!timer)
        abort("Failed to create timer");

    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();

    al_set_new_display_flags(ALLEGRO_WINDOWED);
    display = al_create_display(WINDOW_WIDTH, WINDOW_HEIGHT);
    if (!display)
        abort("Failed to create display");
 
    event_queue = al_create_event_queue();
    if (!event_queue)
        abort("Failed to create event queue");
 
    al_register_event_source(event_queue, al_get_keyboard_event_source());
    al_register_event_source(event_queue, al_get_timer_event_source(timer));
    al_register_event_source(event_queue, al_get_display_event_source(display));
    
    font = al_load_ttf_font("data/DejaVuSans.ttf", 16, 0);
    if (!font)
        abort("Failed to load font!");

    done = false;
}

void Game::init_game(void){
    ship = new Ship(WINDOW_WIDTH/2, WINDOW_HEIGHT/2, FUEL_START, SHIP_MASS);
    add_space(0, 0);
    adjust_ship_position();
    biases.load();
}

void Game::adjust_ship_position(void){
    Collision collision;
    float posx, posy;
    do {
        for (int i=0; i < spaces[space_index].body_count; i++){
            collision = ship->collides(spaces[space_index].bodies[i]);
            if (collision.collides){
                posx = rand() % (int)(WINDOW_WIDTH - ship->width);
                posy = rand() % (int)(WINDOW_HEIGHT - ship->height);
                if (posx - ship->width < 0) posx += ship->width;
                if (posy - ship->height < 0) posy += ship->height;
                ship->pos.x = posx;
                ship->pos.y = posy;
                ship->computeRect();
                break;
            }
        }
    } while(collision.collides);    
}

void Game::add_space(int coordx, int coordy){
    Space space = Space(rand()%10 + 1, coordx, coordy);
    space.init();
    spaces.push_back(space); 
    space_index = spaces.size() - 1;
}  

void Game::update_graphics(void){
    spaces[space_index].draw();
    ship->draw();
    draw_info();
    for (int i=0; i < spaces[space_index].duders.size(); i++)
        draw_duder_bias(&spaces[space_index].duders[i]);
}

void Game::update_game(void){
    get_space_index();
    
    if (space_index < 0)
        add_space(coordx, coordy);

    ship->update();
    update_space();
    collide_ship_bodies();
    collide_ship_loot();
    collide_ship_duder();
}

void Game::update_space(void){
    if (ship->pos.x > WINDOW_WIDTH){
        ship->pos.x = ship->width2;
        coordx += 1;
    }
    if (ship->pos.x < 0){
        ship->pos.x = WINDOW_WIDTH - ship->width2;
        coordx -= 1;
    }
    if (ship->pos.y < 0){
        ship->pos.y = WINDOW_HEIGHT - ship->height2;
        coordy += 1;
    }
    if (ship->pos.y > WINDOW_HEIGHT){
        ship->pos.y = ship->height2;
        coordy -= 1;
    }

    // fix falling through Earth.
    if (ship->pos.y + ship->height2 > WINDOW_HEIGHT - EARTH_HEIGHT && coordy == 0){
        ship->pos.y = WINDOW_HEIGHT - EARTH_HEIGHT - ship->height2;
    }

    for (int i=0; i < spaces[space_index].duders.size() ; i++)
        spaces[space_index].duders[i].update();

}

int Game::get_space_index(void){
    space_index = -1;
    for (int i = 0; i < spaces.size(); i++){
        if (spaces[i].coordx == coordx && spaces[i].coordy == coordy){
            space_index = i;
            break;
        }
    }
    return space_index;
}

void Game::draw_info(void){
    ALLEGRO_COLOR color = al_map_rgb(255, 255, 255);
    float speed = sqrt(ship->vel.x*ship->vel.x + ship->vel.y*ship->vel.y);
    al_draw_textf(font, color, 10, 10, 0,
        "Space Coordinate: (%d, %d) | Spaces Discovered: %ld | Speed: %.1f | Fuel: %.1f", 
        coordx, coordy, spaces.size(), speed, ship->fuel
    );
}

void Game::collide_ship_bodies(void){
    for (int i=0; i < spaces[space_index].body_count; i++){
        Body *body = spaces[space_index].bodies[i];
        Collision collision = ship->collides(body);
        if (collision.collides){
            Collision *prev = &(body->prev_collision);
            if (!prev->collides){
                if (!prev->top || !prev->bottom){
                    if (!prev->top) 
                        ship->pos.y = body->rect.br.y + ship->height2 + 0.1;
                    if (!prev->bottom) 
                        ship->pos.y = body->rect.tl.y - ship->height2 - 0.1;
                    ship->vel.y = 0;
                }
                if (!prev->left || !prev->right){
                    if (!prev->left) 
                        ship->pos.x = body->rect.tl.x - ship->width2 - 0.1;
                    if (!prev->right) 
                        ship->pos.x = body->rect.br.x + ship->width2 + 0.1;
                    ship->vel.x = 0;
                }      
            }
        } else {
            body->prev_collision = collision;
        }
    }
}

void Game::collide_ship_loot(void){
    Space *curr_space = &spaces[space_index];

    for (int i=0; i < curr_space->loots.size(); i++){
        Collision collision = ship->collides(&curr_space->loots[i]);
        if (collision.collides){
            apply_loot(&curr_space->loots[i]);
            curr_space->loots.erase(curr_space->loots.begin()+i);
        } else {
            curr_space->loots[i].prev_collision = collision;
        }
    }
}

void Game::collide_ship_duder(void){
    Space *curr_space = &spaces[space_index];

    for (int i=0; i < curr_space->duders.size(); i++){
        if (curr_space->duders[i].is_killed) continue;

        Collision collision = ship->collides(&curr_space->duders[i]);
        if (collision.collides){
            int count = 0;
            map<string, string>::iterator it;
            for (it=biases.biases.begin(); it != biases.biases.end(); it++){
                if (count == curr_space->duders[i].random_val % biases.biases.size())
                    break;
                count++;
            }
            curr_space->duders[i].bias = *it;
            curr_space->duders[i].is_killed = true;
            cout << curr_space->duders[i].bias.first << endl;
        } else {
            curr_space->duders[i].prev_collision = collision;
        }
    }
}

void Game::draw_duder_bias(Duder *duder){
    if (duder->is_killed){
        size_t bufsize = duder->bias.first.size() + duder->bias.second.size() + 10;
        char buf[bufsize];
        sprintf(buf, "%s:\n%s", 
            duder->bias.first.c_str(), duder->bias.second.c_str());

        al_draw_text(
            font, duder->color, duder->pos.x, 
            duder->pos.y, 0, buf
        );
    }
}
    
void Game::apply_loot(Loot *loot){
    switch(loot->type){
        case FUEL:
            ship->fuel += loot->value;
            ship->fuel = min(ship->fuel_start, ship->fuel);
            break;
        case BOOST:
            ship->vel.x *= loot->value;
            ship->vel.y *= loot->value;
            break;
    }
}

void Game::loop(void){
    bool redraw = true;
    al_start_timer(timer);
    ALLEGRO_KEYBOARD_STATE keys;
    while (!done) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);
        al_get_keyboard_state(&keys);
        
        if (event.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;
            update_game();
        }

        if (al_key_down(&keys, ALLEGRO_KEY_UP))
            ship->thrust_vertical(-1);

        if (al_key_down(&keys, ALLEGRO_KEY_DOWN))
            ship->thrust_vertical(1);

        if (al_key_down(&keys, ALLEGRO_KEY_RIGHT))
            ship->thrust_horizontal(1);

        if (al_key_down(&keys, ALLEGRO_KEY_LEFT))
            ship->thrust_horizontal(-1);

        if (al_key_down(&keys, ALLEGRO_KEY_ESCAPE))
            done = true;
        
        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;
            al_clear_to_color(al_map_rgb(0, 0, 0));
            update_graphics();
            al_flip_display();
        }
    }
}

void Game::abort(const char* message){
    printf("%s \n", message);
    shutdown();
    exit(1);
}

void Game::shutdown(void){
    if (timer)
        al_destroy_timer(timer);
 
    if (display)
        al_destroy_display(display);
 
    if (event_queue)
        al_destroy_event_queue(event_queue);

    al_shutdown_primitives_addon();
}
