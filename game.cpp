#include <allegro5/allegro5.h>
#include <allegro5/allegro_primitives.h>
#include <stdio.h>
#include <stdlib.h>

#include "defines.h"
#include "game.h"

void Game::abort(const char* message)
{
    printf("%s \n", message);
    shutdown();
    exit(1);
}
 
void Game::init_graphics(void)
{
    if (!al_init())
        abort("Failed to initialize allegro");
 
    if (!al_install_keyboard())
        abort("Failed to install keyboard");
 
    timer = al_create_timer(1.0 / 60);
    if (!timer)
        abort("Failed to create timer");
 
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
    al_init_primitives_addon();
    done = false;
}
 
void Game::shutdown(void)
{
    if (timer)
        al_destroy_timer(timer);
 
    if (display)
        al_destroy_display(display);
 
    if (event_queue)
        al_destroy_event_queue(event_queue);
}
 
void Game::update_graphics()
{
    al_draw_line(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT, al_map_rgb(255, 255, 2), 10);
}

void Game::loop(void)
{
    bool redraw = true;
    al_start_timer(timer);
 
    while (!done) {
        ALLEGRO_EVENT event;
        al_wait_for_event(event_queue, &event);
 
        if (event.type == ALLEGRO_EVENT_TIMER) {
            redraw = true;
            //update_logic();
        }
        else if (event.type == ALLEGRO_EVENT_KEY_DOWN) {
            if (event.keyboard.keycode == ALLEGRO_KEY_ESCAPE) {
                done = true;
            }
            //get_user_input();
        }
 
        if (redraw && al_is_event_queue_empty(event_queue)) {
            redraw = false;
            al_clear_to_color(al_map_rgb(0, 0, 0));
            update_graphics();
            al_flip_display();
        }
    }
}
