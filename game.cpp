#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_ttf.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/allegro_audio.h>
#include <allegro5/allegro_acodec.h>

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
#include "starfield.h"

using namespace std;

Game::Game()
:coordx(0), coordy(0), space_index(-1), done(false), difficulty(1){
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
    
    ALLEGRO_MONITOR_INFO info;
    if (al_get_monitor_info(0, &info)){
        window_width = (int)info.x2*0.75;
        window_height = (int)info.y2*0.75;
    }
   
    timer = al_create_timer(FRAME_RATE);
    if (!timer)
        abort("Failed to create timer");

    al_init_primitives_addon();
    al_init_font_addon();
    al_init_ttf_addon();
    al_init_acodec_addon();
    
    if (!al_install_audio()) {
        abort("Could not init sound.\n");
    }

    voice = al_create_voice(44100, ALLEGRO_AUDIO_DEPTH_INT16,
        ALLEGRO_CHANNEL_CONF_2);
    if (!voice) {
        abort("Could not create voice.\n");
    }

    mixer = al_create_mixer(44100, ALLEGRO_AUDIO_DEPTH_FLOAT32,
        ALLEGRO_CHANNEL_CONF_2);
    if (!mixer) {
        abort("Could not create mixer.\n");
    }

    if (!al_attach_mixer_to_voice(mixer, voice)) {
        abort("al_attach_mixer_to_voice failed.\n");
    }

    stream = al_load_audio_stream("./data/Power_Glove-Clutch.ogg", 4, 2048);
    if (music_on){
        al_set_audio_stream_playmode(stream, ALLEGRO_PLAYMODE_LOOP);
        al_attach_audio_stream_to_mixer(stream, mixer);
    }

    al_set_new_display_flags(ALLEGRO_WINDOWED);
    // al_set_new_display_option(ALLEGRO_SWAP_METHOD, 2, ALLEGRO_SUGGEST);
    // al_set_new_display_option(ALLEGRO_VSYNC, 1, ALLEGRO_SUGGEST);
    // al_set_new_display_refresh_rate(60);
    display = al_create_display(window_width, window_height);
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
    ship = new Ship(window_width/2, window_height/2, FUEL_START, SHIP_MASS);
    add_space(0, 0);
    adjust_ship_position();
    biases.load();
    starfield.init(window_width, window_height);
}

void Game::adjust_ship_position(void){
    Collision collision;
    float posx, posy;
    do {
        for (int i=0; i < spaces[space_index].body_count; i++){
            collision = ship->collides(spaces[space_index].bodies[i]);
            if (collision.collides){
                posx = rand() % (int)(window_width - ship->width);
                posy = rand() % (int)(window_height - ship->height);
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
    int num = rand() % (10*((10-difficulty) + 1));
    bool gravitate_bodies = (num < 1);
    //cout << num << " " << gravitate_bodies <<endl;
    Space space = Space(rand()%BODY_COUNT + 1, coordx, coordy, window_width, window_height, gravitate_bodies);
    space.init(difficulty);
    spaces.push_back(space); 
    space_index = spaces.size() - 1;
}  

void Game::update_graphics(void){
    starfield.draw();
    spaces[space_index].draw();
    ship->draw();
    for (int i=0; i < spaces[space_index].duders.size(); i++)
        draw_duder_bias(&spaces[space_index].duders[i]);
    draw_info();
}

void Game::update_game(ALLEGRO_EVENT &e){
    get_space_index();
    
    if (space_index < 0)
        add_space(coordx, coordy);

    starfield.update();
    ship->gravitate_bodies(spaces[space_index]);
    ship->update(e);

    collide_duder_bodies();
    collide_ship_bodies();
    collide_ship_loot();
    collide_ship_duder();
    update_space();
}

void Game::update_space(void){
    if (ship->pos.x > window_width){
        ship->pos.x = ship->width2;
        coordx += 1;
    }
    if (ship->pos.x < 0){
        ship->pos.x = window_width - ship->width2;
        coordx -= 1;
    }
    if (ship->pos.y < 0){
        ship->pos.y = window_height - ship->height2;
        coordy += 1;
    }
    if (ship->pos.y > window_height){
        ship->pos.y = ship->height2;
        coordy -= 1;
    }

    // fix falling through Earth.
    if (ship->pos.y + ship->height2 > window_height - EARTH_HEIGHT && coordy == 0){
        ship->pos.y = window_height - EARTH_HEIGHT - ship->height2;
        ship->vel.y = 0;
    }

    for (int i=0; i < spaces[space_index].duders.size() ; i++)
        spaces[space_index].duders[i].update(window_width, window_height);

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
        "Space Coordinate: (%d, %d) | Spaces Discovered: %ld | Biases Groked: %ld/%ld | Speed: %.1f | Fuel: %.1f", 
        coordx, coordy, spaces.size(), biases_groked.size(), biases.biases.size(), speed, ship->fuel
    );
}

void Game::collide_ship_bodies(void){
    for (int i=0; i < spaces[space_index].body_count; i++){
        Body *body = spaces[space_index].bodies[i];
        Collision collision = ship->collides(body);
        if (collision.collides){
            Collision *prev = &(body->prev_collision_map[ship]);
            if (!prev->collides){
                if (!prev->top || !prev->bottom){
                    if (!prev->top) 
                        ship->pos.y = body->rect.br.y + ship->height2 + 0.01;
                    if (!prev->bottom) 
                        ship->pos.y = body->rect.tl.y - ship->height2 - 0.01;
                    ship->vel.y = 0;
                }
                if (!prev->left || !prev->right){
                    if (!prev->left) 
                        ship->pos.x = body->rect.tl.x - ship->width2 - 0.01;
                    if (!prev->right) 
                        ship->pos.x = body->rect.br.x + ship->width2 + 0.01;
                    ship->vel.x = 0;
                }      
            }
        } else {
            body->prev_collision_map[ship] = collision;
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
            curr_space->loots[i].prev_collision_map[ship] = collision;
        }
    }
}

void Game::collide_ship_duder(void){
    Space *curr_space = &spaces[space_index];

    for (int i=0; i < curr_space->duders.size(); i++){
        if (curr_space->duders[i].is_killed) continue;

        Collision collision = ship->collides(&curr_space->duders[i]);
        if (collision.collides){
            int count = 0, mod = curr_space->duders[i].random_val % biases.biases.size();
            map<string, string>::iterator it;
            for (it=biases.biases.begin(); it != biases.biases.end(); it++){
                if (count == mod)
                    break;
                count++;
            }
            curr_space->duders[i].bias = &(*it);
            curr_space->duders[i].is_killed = true;
            cout << curr_space->duders[i].bias->first << endl;
            biases_groked.insert(curr_space->duders[i].bias->first);
        } else {
            curr_space->duders[i].prev_collision_map[ship] = collision;
        }
    }
}

void Game::collide_duder_bodies(void){
    Space *curr_space = &spaces[space_index];

    for (int i=0; i < curr_space->duders.size(); i++){
        if (curr_space->duders[i].is_killed) continue;
        for (int j=0; j < curr_space->body_count; j++){
            Collision collision = curr_space->duders[i].collides(curr_space->bodies[j]);
            if (collision.collides){
                Collision *prev = &(curr_space->duders[i].prev_collision_map[curr_space->bodies[j]]);
            
                if (!prev->collides){
                    if (!prev->top || !prev->bottom){
                        curr_space->duders[i].vel.y = -curr_space->duders[i].vel.y;
                    }
                    if (!prev->left || !prev->right){
                        curr_space->duders[i].vel.x = -curr_space->duders[i].vel.x;
                    }
                }
            } else {
                curr_space->duders[i].prev_collision_map[curr_space->bodies[j]] = collision;
            }
        }
    }
}

void Game::draw_duder_bias(Duder *duder){
    if (!duder->is_killed) return;

    char buf[500];
    memset(buf, 0, sizeof(buf));
    int countword = 0, lastbr = 0, linenum = 0;
    string txt = duder->bias->second;
    int maxwords = 7;
    int tw = 0;
    float posx, posy = duder->pos.y - 60;
    
    for (int i=0; i < txt.size(); i++){
        if (txt[i] == ' '){
            countword++;
        }
        if (countword >= maxwords){
            lastbr = i;
            linenum++;
            countword = 0;
            if (!tw){
                tw = al_get_text_width(font, buf) + 30;
                posx = duder->pos.x - tw/2;
                if (posx+tw > window_width) posx = window_width-tw;
                if (posx < 0) posx = 30;
            }
            al_draw_text(
                font, duder->color, posx, 
                posy+linenum*20, 0, buf
            );
            memset(buf, 0, sizeof(buf));
        }
        
        if (i-lastbr < sizeof(buf))
            buf[i-lastbr] = txt[i]; 
        else
            countword = maxwords + 1;
    }
    //draw last line
    linenum++;
    al_draw_text(
        font, duder->color, posx, 
        posy+linenum*20, 0, buf
    );

    sprintf(buf, "%s:", duder->bias->first.c_str());
    al_draw_text(
        font, duder->color, posx, 
        posy, 0, buf
    );
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
            update_game(event);
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

    al_destroy_audio_stream(stream);
    al_destroy_mixer(mixer);
    al_destroy_voice(voice);
    al_uninstall_audio();

    al_shutdown_primitives_addon();
}
