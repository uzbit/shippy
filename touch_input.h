#ifndef _TOUCH_INPUT_H_
#define _TOUCH_INPUT_H_

#include <SDL3/SDL.h>

struct TouchState {
    float joystick_x;    // -1.0 to 1.0 (rotation)
    float joystick_y;    // -1.0 to 1.0 (thrust when negative/up)
    bool fire_pressed;
    bool brake_pressed;
};

class TouchInput {
public:
    TouchInput();
    void init(int screen_width, int screen_height);
    void handle_event(const SDL_Event& event);
    void draw();

    TouchState state;

private:
    int screen_w, screen_h;

    // Joystick tracking
    SDL_FingerID joystick_finger;
    bool joystick_active;
    float joystick_origin_x, joystick_origin_y; // where finger first touched (pixels)
    float joystick_current_x, joystick_current_y;
    float joystick_max_radius; // max displacement in pixels

    // Button zones (in pixels)
    SDL_FRect fire_zone;
    SDL_FRect brake_zone;

    bool point_in_zone(float x, float y, const SDL_FRect& zone);
    void draw_joystick_overlay();
    void draw_button_overlays();
};

#endif
