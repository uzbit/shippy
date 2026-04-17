#include "touch_input.h"
#include "sdl_compat.h"
#include <cmath>
#include <algorithm>

TouchInput::TouchInput()
    : screen_w(0), screen_h(0),
      joystick_finger(0), joystick_active(false),
      joystick_origin_x(0), joystick_origin_y(0),
      joystick_current_x(0), joystick_current_y(0),
      joystick_max_radius(0) {
    state = {0, false, false};
    fire_zone = {0, 0, 0, 0};
    thrust_zone = {0, 0, 0, 0};
}

void TouchInput::init(int screen_width, int screen_height) {
    screen_w = screen_width;
    screen_h = screen_height;
    joystick_max_radius = screen_w * 0.08f;

    // Button zones: bottom-right corner
    float btn_size = screen_h * 0.18f;
    float margin = screen_w * 0.02f;
    float bottom = screen_h - margin;

    // Fire button: bottom-right
    fire_zone = {
        screen_w - margin - btn_size,
        bottom - btn_size,
        btn_size,
        btn_size
    };

    // Thrust button: left of fire
    thrust_zone = {
        screen_w - margin * 2 - btn_size * 2,
        bottom - btn_size,
        btn_size,
        btn_size
    };
}

bool TouchInput::point_in_zone(float x, float y, const SDL_FRect& zone) {
    return x >= zone.x && x < zone.x + zone.w && y >= zone.y && y < zone.y + zone.h;
}

void TouchInput::handle_event(const SDL_Event& event) {
    if (event.type == SDL_EVENT_FINGER_DOWN) {
        float px = event.tfinger.x * screen_w;
        float py = event.tfinger.y * screen_h;

        if (point_in_zone(px, py, fire_zone)) {
            state.fire_pressed = true;
        } else if (point_in_zone(px, py, thrust_zone)) {
            state.thrust_pressed = true;
        } else if (px < screen_w * 0.45f) {
            // Left side of screen: joystick
            joystick_active = true;
            joystick_finger = event.tfinger.fingerID;
            joystick_origin_x = px;
            joystick_origin_y = py;
            joystick_current_x = px;
            joystick_current_y = py;
        }
    }

    if (event.type == SDL_EVENT_FINGER_MOTION) {
        float px = event.tfinger.x * screen_w;
        float py = event.tfinger.y * screen_h;

        if (joystick_active && event.tfinger.fingerID == joystick_finger) {
            joystick_current_x = px;
            joystick_current_y = joystick_origin_y; // horizontal-only

            float dx = joystick_current_x - joystick_origin_x;

            if (dx > joystick_max_radius) dx = joystick_max_radius;
            if (dx < -joystick_max_radius) dx = -joystick_max_radius;
            joystick_current_x = joystick_origin_x + dx;

            float deadzone = 0.15f;
            state.joystick_x = dx / joystick_max_radius;
            if (fabsf(state.joystick_x) < deadzone) state.joystick_x = 0;
        }

        // Update button press state for held fingers that move
        if (point_in_zone(px, py, fire_zone)) {
            state.fire_pressed = true;
        }
    }

    if (event.type == SDL_EVENT_FINGER_UP) {
        float px = event.tfinger.x * screen_w;
        float py = event.tfinger.y * screen_h;

        if (joystick_active && event.tfinger.fingerID == joystick_finger) {
            joystick_active = false;
            state.joystick_x = 0;
        }

        if (point_in_zone(px, py, fire_zone) ||
            (!joystick_active && event.tfinger.fingerID != joystick_finger)) {
            state.fire_pressed = false;
        }
        if (point_in_zone(px, py, thrust_zone)) {
            state.thrust_pressed = false;
        }
    }
}

void TouchInput::draw_joystick_overlay() {
    GameColor dim = {1.0f, 1.0f, 1.0f, 0.15f};
    GameColor bright = {1.0f, 1.0f, 1.0f, 0.3f};

    if (joystick_active) {
        // Outer ring at origin
        draw_ellipse(joystick_origin_x, joystick_origin_y,
                     joystick_max_radius, joystick_max_radius, dim, 2.0f);
        // Inner dot at current position
        draw_filled_ellipse(joystick_current_x, joystick_current_y,
                           15, 15, bright);
    }
}

void TouchInput::draw_button_overlays() {
    GameColor fire_color = state.fire_pressed ?
        GameColor{1.0f, 0.3f, 0.3f, 0.4f} :
        GameColor{1.0f, 0.3f, 0.3f, 0.15f};
    GameColor thrust_color = state.thrust_pressed ?
        GameColor{0.3f, 1.0f, 0.3f, 0.4f} :
        GameColor{0.3f, 1.0f, 0.3f, 0.15f};

    // Fire button
    float fcx = fire_zone.x + fire_zone.w / 2;
    float fcy = fire_zone.y + fire_zone.h / 2;
    float fr = fire_zone.w / 2 * 0.85f;
    draw_filled_ellipse(fcx, fcy, fr, fr, fire_color);
    draw_ellipse(fcx, fcy, fr, fr, GameColor{1.0f, 0.3f, 0.3f, 0.3f}, 2.0f);

    // Thrust button
    float tcx = thrust_zone.x + thrust_zone.w / 2;
    float tcy = thrust_zone.y + thrust_zone.h / 2;
    float tr = thrust_zone.w / 2 * 0.85f;
    draw_filled_ellipse(tcx, tcy, tr, tr, thrust_color);
    draw_ellipse(tcx, tcy, tr, tr, GameColor{0.3f, 1.0f, 0.3f, 0.3f}, 2.0f);
}

void TouchInput::draw() {
    draw_joystick_overlay();
    draw_button_overlays();
}
