
#include <cmath>
#include "sdl_compat.h"
#include "starfield.h"

void Starfield::init(int w, int h) {
    stars.clear();
    nebulas.clear();
    window_width = w;
    window_height = h;

    // 3 parallax layers of stars
    num_stars = 250 + rand() % 100;

    for (int i = 0; i < num_stars; i++) {
        Star s;
        s.x = (float)(rand() % (window_width * 3)) - window_width;
        s.y = (float)(rand() % (window_height * 3)) - window_height;
        s.twinkle_speed = 0.5f + (rand() % 300) / 100.0f;
        s.twinkle_phase = (rand() % 1000) / 1000.0f * 6.28f;

        int layer = rand() % 100;
        if (layer < 55) {
            // Far: small, dimmer, slow parallax
            s.depth = 0.05f + (rand() % 10) / 100.0f;
            s.size = 1.0f + (rand() % 10) / 10.0f;
            s.brightness = 0.5f + (rand() % 30) / 100.0f;
        } else if (layer < 85) {
            // Mid: medium
            s.depth = 0.15f + (rand() % 15) / 100.0f;
            s.size = 2.0f + (rand() % 20) / 10.0f;
            s.brightness = 0.7f + (rand() % 20) / 100.0f;
        } else {
            // Near: large, bright
            s.depth = 0.3f + (rand() % 20) / 100.0f;
            s.size = 4.0f + (rand() % 30) / 10.0f;
            s.brightness = 0.85f + (rand() % 15) / 100.0f;
        }

        // Real star colors
        int color_type = rand() % 100;
        if (color_type < 45) {
            // White/blue-white
            s.color = map_rgb_f(0.9f, 0.93f, 1.0f);
        } else if (color_type < 70) {
            // Warm yellow
            s.color = map_rgb_f(1.0f, 0.92f, 0.7f);
        } else if (color_type < 88) {
            // Blue
            s.color = map_rgb_f(0.6f, 0.7f, 1.0f);
        } else {
            // Red/orange
            s.color = map_rgb_f(1.0f, 0.5f, 0.3f);
        }

        stars.push_back(s);
    }

    // Nebula clouds — much more visible
    int num_nebulas = 6 + rand() % 4;
    for (int i = 0; i < num_nebulas; i++) {
        Nebula n;
        n.x = (float)(rand() % (window_width * 3)) - window_width;
        n.y = (float)(rand() % (window_height * 3)) - window_height;
        n.radius = 200.0f + (rand() % 400);
        n.depth = 0.02f + (rand() % 5) / 100.0f;

        int ncolor = rand() % 5;
        switch (ncolor) {
            case 0: n.color = {0.4f, 0.1f, 0.6f, 0.15f}; break;  // purple
            case 1: n.color = {0.1f, 0.2f, 0.5f, 0.12f}; break;  // deep blue
            case 2: n.color = {0.5f, 0.1f, 0.2f, 0.12f}; break;  // red
            case 3: n.color = {0.1f, 0.4f, 0.4f, 0.12f}; break;  // teal
            case 4: n.color = {0.3f, 0.4f, 0.1f, 0.10f}; break;  // green
        }

        nebulas.push_back(n);
    }
}

void Starfield::update(void) {
}

void Starfield::draw(void) {
    float t = SDL_GetTicks() * 0.001f;
    float cam_ox = g_camera_x - g_screen_cx;
    float cam_oy = g_camera_y - g_screen_cy;

    SDL_SetRenderDrawBlendMode(g_renderer, SDL_BLENDMODE_BLEND);

    // Draw nebulas — layered gradient circles
    for (auto& n : nebulas) {
        float px = n.x - cam_ox * n.depth;
        float py = n.y - cam_oy * n.depth;

        // Wrap
        float ww = window_width * 2.0f;
        float wh = window_height * 2.0f;
        px = fmodf(px + ww, ww);
        py = fmodf(py + wh, wh);
        if (px > window_width + n.radius) px -= ww;
        if (py > window_height + n.radius) py -= wh;

        // Skip if far off-screen
        if (px < -n.radius * 1.5f || px > window_width + n.radius * 1.5f ||
            py < -n.radius * 1.5f || py > window_height + n.radius * 1.5f)
            continue;

        // Draw as gradient triangle fan — bright center fading out
        int segments = 32;
        for (int s = 0; s < segments; s++) {
            float a1 = s * 2.0f * M_PI / segments;
            float a2 = (s + 1) * 2.0f * M_PI / segments;

            SDL_Vertex verts[3];
            // Center: full color
            verts[0].position = {px, py};
            verts[0].color = {n.color.r, n.color.g, n.color.b, n.color.a};
            // Edges: transparent
            verts[1].position = {px + cosf(a1) * n.radius, py + sinf(a1) * n.radius};
            verts[1].color = {n.color.r, n.color.g, n.color.b, 0.0f};
            verts[2].position = {px + cosf(a2) * n.radius, py + sinf(a2) * n.radius};
            verts[2].color = {n.color.r, n.color.g, n.color.b, 0.0f};
            SDL_RenderGeometry(g_renderer, NULL, verts, 3, NULL, 0);
        }
    }

    // Draw stars
    for (auto& s : stars) {
        float px = s.x - cam_ox * s.depth;
        float py = s.y - cam_oy * s.depth;

        // Wrap
        float ww = window_width * 2.0f;
        float wh = window_height * 2.0f;
        px = fmodf(px + ww, ww);
        py = fmodf(py + wh, wh);
        if (px > window_width + 10) px -= ww;
        if (py > window_height + 10) py -= wh;

        if (px < -10 || px > window_width + 10 || py < -10 || py > window_height + 10)
            continue;

        // Twinkle
        float twinkle = 0.6f + 0.4f * sinf(t * s.twinkle_speed + s.twinkle_phase);
        float bright = s.brightness * twinkle;

        if (s.size <= 1.5f) {
            // Small: solid dot
            SDL_SetRenderDrawColorFloat(g_renderer,
                s.color.r * bright, s.color.g * bright, s.color.b * bright, bright);
            SDL_FRect r = {px - 0.5f, py - 0.5f, s.size, s.size};
            SDL_RenderFillRect(g_renderer, &r);
        } else if (s.size <= 3.0f) {
            // Medium: cross sparkle
            float sz = s.size * twinkle;
            SDL_SetRenderDrawColorFloat(g_renderer,
                s.color.r * bright, s.color.g * bright, s.color.b * bright, bright);
            SDL_FRect core = {px - 1, py - 1, 2, 2};
            SDL_RenderFillRect(g_renderer, &core);
            SDL_FRect h = {px - sz, py, sz * 2, 1};
            SDL_FRect v = {px, py - sz, 1, sz * 2};
            SDL_RenderFillRect(g_renderer, &h);
            SDL_RenderFillRect(g_renderer, &v);
        } else {
            // Large: glow + core + spikes
            float sz = s.size;

            // Outer glow — triangle fan with gradient
            int segments = 16;
            float glow_r = sz * 2.5f;
            for (int i = 0; i < segments; i++) {
                float a1 = i * 2.0f * M_PI / segments;
                float a2 = (i + 1) * 2.0f * M_PI / segments;
                SDL_Vertex verts[3];
                verts[0].position = {px, py};
                verts[0].color = {s.color.r * bright, s.color.g * bright, s.color.b * bright, bright * 0.4f};
                verts[1].position = {px + cosf(a1) * glow_r, py + sinf(a1) * glow_r};
                verts[1].color = {s.color.r, s.color.g, s.color.b, 0.0f};
                verts[2].position = {px + cosf(a2) * glow_r, py + sinf(a2) * glow_r};
                verts[2].color = {s.color.r, s.color.g, s.color.b, 0.0f};
                SDL_RenderGeometry(g_renderer, NULL, verts, 3, NULL, 0);
            }

            // Bright core
            SDL_SetRenderDrawColorFloat(g_renderer,
                s.color.r, s.color.g, s.color.b, bright);
            SDL_FRect core = {px - sz * 0.4f, py - sz * 0.4f, sz * 0.8f, sz * 0.8f};
            SDL_RenderFillRect(g_renderer, &core);

            // Cross spikes
            SDL_FRect ch = {px - sz * 1.5f, py - 0.5f, sz * 3, 1};
            SDL_FRect cv = {px - 0.5f, py - sz * 1.5f, 1, sz * 3};
            SDL_RenderFillRect(g_renderer, &ch);
            SDL_RenderFillRect(g_renderer, &cv);
        }
    }
}
