
#include <iostream>

#include "sdl_compat.h"
#include "duder.h"
#include "geom.h"
#include "defines.h"
#include "physics_world.h"

using namespace std;

Duder::Duder(float x, float y, float width, float height)
:Object(x, y, width, height){
    pos.x = x;
    pos.y = y;
    vel.x = ((rand() % 300) - 150)/50.0;
    vel.y = ((rand() % 300) - 150)/50.0;
    color = map_rgb(rand()%255, rand()%255, rand()%255);
    random_val = rand();
    thick = rand() % 10  +1;
    angle = 0.0f;
    is_killed = false;
    is_following = false;
    is_launched = false;
    launch_life = 0.0f;
}

void Duder::initPhysics(PhysicsWorld& world) {
    physicsWorldPtr = &world;
    // Use DYNAMIC instead of KINEMATIC so duders collide with static bodies
    physicsBody = world.createBody(this, PhysicsBodyType::DYNAMIC, 0.5f, 0.0f, 1.0f);
    if (b2Body_IsValid(physicsBody)) {
        // Disable gravity for duders so they float
        b2Body_SetGravityScale(physicsBody, 0.0f);
        b2Body_SetFixedRotation(physicsBody, false);
        b2Body_SetLinearDamping(physicsBody, 0.0f);
        // vel is pixels/frame, convert to pixels/second for physics
        world.setLinearVelocity(physicsBody, vel.x * FRAME_RATE, vel.y * FRAME_RATE);
    }
}

void Duder::syncFromPhysics() {
    if (!physicsWorldPtr || !b2Body_IsValid(physicsBody)) return;

    b2Vec2 physPos = physicsWorldPtr->getPosition(physicsBody);
    b2Vec2 physVel = physicsWorldPtr->getLinearVelocity(physicsBody);
    pos.x = physPos.x;
    pos.y = physPos.y;
    vel.x = physVel.x / FRAME_RATE;
    vel.y = physVel.y / FRAME_RATE;

    b2Rot rot = b2Body_GetRotation(physicsBody);
    angle = atan2f(rot.s, rot.c);
}

void Duder::update(int w, int h, float target_x, float target_y, float target_vx, float target_vy){
    if (is_killed) return;

    if (is_launched) {
        launch_life -= 1.0f / 60.0f;
        if (launch_life <= 0.0f) {
            is_killed = true;
            return;
        }
        if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
            syncFromPhysics();
        } else {
            pos.x += vel.x;
            pos.y += vel.y;
        }
        return;
    }

    if (is_following) {
        float speed = sqrtf(target_vx * target_vx + target_vy * target_vy);
        float drag_dist = 60.0f + (random_val % 80);

        // Per-duder spread angle: fan out in a cone behind the ship
        float spread_angle = ((random_val % 1000) / 1000.0f - 0.5f) * 1.6f; // -0.8 to +0.8 radians

        float goal_x, goal_y;
        if (speed > 0.1f) {
            // Base direction: opposite of velocity
            float base_angle = atan2f(-target_vy, -target_vx);
            float angle = base_angle + spread_angle;
            goal_x = target_x + cosf(angle) * drag_dist;
            goal_y = target_y + sinf(angle) * drag_dist;
        } else {
            // When stopped, spread in a circle around the ship
            float idle_angle = (random_val % 360) * M_PI / 180.0f;
            goal_x = target_x + cosf(idle_angle) * drag_dist * 0.5f;
            goal_y = target_y + sinf(idle_angle) * drag_dist * 0.5f;
        }

        // Wobble orbit around the drag point
        float t_now = SDL_GetTicks() * 0.001f;
        float phase = random_val * 0.7f;
        float wobble_r = 25.0f + (random_val % 40);
        float wx_speed = 1.2f + (random_val % 100) * 0.01f;
        float wy_speed = wx_speed * (0.6f + (random_val % 80) * 0.01f);
        goal_x += cosf(t_now * wx_speed + phase) * wobble_r;
        goal_y += sinf(t_now * wy_speed + phase * 1.3f) * wobble_r;

        // Slow lerp so the wobble path is visible
        pos.x = lerp(pos.x, goal_x, 0.04f);
        pos.y = lerp(pos.y, goal_y, 0.04f);

        if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
            vel.x = (goal_x - pos.x);
            vel.y = (goal_y - pos.y);
            physicsWorldPtr->setLinearVelocity(physicsBody, vel.x * FRAME_RATE, vel.y * FRAME_RATE);
        }
    } else {
        if (physicsWorldPtr && b2Body_IsValid(physicsBody)) {
            syncFromPhysics();
        } else {
            pos.x += vel.x;
            pos.y += vel.y;
        }
    }
}


void Duder::draw(void){
    if (!is_killed){
        float hw = width2;
        float hh = height2;
        GameColor dark = map_rgb(20, 20, 20);
        GameColor white = map_rgb(255, 255, 255);

        // Render duder to a small offscreen texture, then draw it rotated
        int tex_w = (int)(hw * 5);
        int tex_h = (int)(hh * 5);
        if (tex_w < 16) tex_w = 16;
        if (tex_h < 16) tex_h = 16;

        SDL_Texture* duderTex = SDL_CreateTexture(g_renderer, SDL_PIXELFORMAT_RGBA8888,
                                                   SDL_TEXTUREACCESS_TARGET, tex_w, tex_h);
        if (!duderTex) return;
        SDL_SetTextureBlendMode(duderTex, SDL_BLENDMODE_BLEND);

        // Save render state and draw to duder texture
        SDL_Texture* prevTarget = SDL_GetRenderTarget(g_renderer);
        float save_cam_x = g_camera_x, save_cam_y = g_camera_y, save_cam_zoom = g_camera_zoom;
        float save_scx = g_screen_cx, save_scy = g_screen_cy;

        SDL_SetRenderTarget(g_renderer, duderTex);
        SDL_SetRenderDrawColor(g_renderer, 0, 0, 0, 0);
        SDL_RenderClear(g_renderer);

        // Draw centered in the texture (camera maps local coords to texture center)
        float cx = tex_w / 2.0f;
        float cy = tex_h / 2.0f;
        g_camera_x = 0;
        g_camera_y = 0;
        g_camera_zoom = 1.0f;
        g_screen_cx = cx;
        g_screen_cy = cy;

        // Legs
        float leg_spread = hw * 0.6f;
        float leg_len = hh * 0.7f;
        draw_line(-leg_spread, hh * 0.5f,
                  -leg_spread * 1.2f, hh * 0.5f + leg_len, color, thick);
        draw_line(leg_spread, hh * 0.5f,
                  leg_spread * 1.2f, hh * 0.5f + leg_len, color, thick);
        draw_filled_ellipse(-leg_spread * 1.2f, hh * 0.5f + leg_len,
                            hw * 0.25f, hh * 0.15f, color);
        draw_filled_ellipse(leg_spread * 1.2f, hh * 0.5f + leg_len,
                            hw * 0.25f, hh * 0.15f, color);

        // Arms
        float wave = sinf(random_val + pos.x * 0.05f) * hh * 0.3f;
        draw_line(-hw, -hh * 0.1f, -hw * 1.8f, -hh * 0.1f + wave, color, thick);
        draw_line(hw, -hh * 0.1f, hw * 1.8f, -hh * 0.1f - wave, color, thick);
        draw_filled_ellipse(-hw * 1.8f, -hh * 0.1f + wave, hw * 0.18f, hw * 0.18f, color);
        draw_filled_ellipse(hw * 1.8f, -hh * 0.1f - wave, hw * 0.18f, hw * 0.18f, color);

        // Body
        draw_filled_ellipse(0, 0, hw, hh * 0.7f, color);
        draw_ellipse(0, 0, hw, hh * 0.7f, dark, 1.5f);

        // Head
        float head_dy = -hh;
        float head_r = hw * 0.8f;
        draw_filled_ellipse(0, head_dy, head_r, head_r, color);
        draw_ellipse(0, head_dy, head_r, head_r, dark, 1.5f);

        // Eyes
        float eye_off = head_r * 0.35f;
        float eye_r = head_r * 0.22f;
        float pupil_r = eye_r * 0.55f;
        draw_filled_ellipse(-eye_off, head_dy, eye_r, eye_r, white);
        draw_filled_ellipse(eye_off, head_dy, eye_r, eye_r, white);
        draw_filled_ellipse(-eye_off + pupil_r * 0.3f, head_dy, pupil_r, pupil_r, dark);
        draw_filled_ellipse(eye_off + pupil_r * 0.3f, head_dy, pupil_r, pupil_r, dark);

        // Antenna
        if (random_val % 3 != 0) {
            float ant_h = hh * 0.5f;
            float ant_lean = (random_val % 2 == 0) ? hw * 0.2f : -hw * 0.2f;
            draw_line(0, head_dy - head_r,
                      ant_lean, head_dy - head_r - ant_h, color, 2.0f);
            draw_filled_ellipse(ant_lean, head_dy - head_r - ant_h,
                                hw * 0.12f, hw * 0.12f, white);
        }

        // Restore render state
        SDL_SetRenderTarget(g_renderer, prevTarget);
        g_camera_x = save_cam_x;
        g_camera_y = save_cam_y;
        g_camera_zoom = save_cam_zoom;
        g_screen_cx = save_scx;
        g_screen_cy = save_scy;

        // Draw the texture rotated at the duder's world position
        float screen_x = cam_wx(pos.x);
        float screen_y = cam_wy(pos.y);
        float scale = g_camera_zoom;
        SDL_FRect dst = {
            screen_x - tex_w * 0.5f * scale,
            screen_y - tex_h * 0.5f * scale,
            tex_w * scale,
            tex_h * scale
        };
        SDL_RenderTextureRotated(g_renderer, duderTex, NULL, &dst,
                                  angle * 180.0f / M_PI, NULL, SDL_FLIP_NONE);
        SDL_DestroyTexture(duderTex);
    }
}
