#ifndef _SDL_COMPAT_H_
#define _SDL_COMPAT_H_

#include <SDL3/SDL.h>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Type alias for color - SDL3 uses SDL_FColor for floating-point colors
typedef SDL_FColor GameColor;

// Global renderer pointer (set during init)
extern SDL_Renderer* g_renderer;

// Helper function to create a color from RGB values (0-255)
inline GameColor map_rgb(int r, int g, int b) {
    return SDL_FColor{r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
}

// Helper function to create a color from RGB float values (0.0-1.0)
inline GameColor map_rgb_f(float r, float g, float b) {
    return SDL_FColor{r, g, b, 1.0f};
}

// Set the current draw color on the renderer
inline void set_draw_color(const GameColor& color) {
    SDL_SetRenderDrawColorFloat(g_renderer, color.r, color.g, color.b, color.a);
}

// Draw a filled rectangle
inline void draw_filled_rect(float x1, float y1, float x2, float y2, const GameColor& color) {
    set_draw_color(color);
    SDL_FRect rect = {x1, y1, x2 - x1, y2 - y1};
    SDL_RenderFillRect(g_renderer, &rect);
}

// Draw a line
inline void draw_line(float x1, float y1, float x2, float y2, const GameColor& color, float thickness) {
    set_draw_color(color);
    // SDL3 doesn't have built-in thick lines, so we draw a thin line
    // For thicker lines, we'd need to use SDL_RenderGeometry with a quad
    SDL_RenderLine(g_renderer, x1, y1, x2, y2);
    (void)thickness; // TODO: implement thick lines if needed
}

// Draw a filled triangle using SDL_RenderGeometry
inline void draw_filled_triangle(float x1, float y1, float x2, float y2, float x3, float y3, const GameColor& color) {
    SDL_Vertex vertices[3];

    vertices[0].position.x = x1;
    vertices[0].position.y = y1;
    vertices[0].color = color;

    vertices[1].position.x = x2;
    vertices[1].position.y = y2;
    vertices[1].color = color;

    vertices[2].position.x = x3;
    vertices[2].position.y = y3;
    vertices[2].color = color;

    SDL_RenderGeometry(g_renderer, NULL, vertices, 3, NULL, 0);
}

// Draw a filled ellipse (approximation using triangles)
inline void draw_filled_ellipse(float cx, float cy, float rx, float ry, const GameColor& color) {
    const int segments = 32;
    SDL_Vertex vertices[segments + 2];

    // Center vertex
    vertices[0].position.x = cx;
    vertices[0].position.y = cy;
    vertices[0].color = color;

    // Circle vertices
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i * 2.0f * M_PI / (float)segments;
        vertices[i + 1].position.x = cx + rx * cosf(angle);
        vertices[i + 1].position.y = cy + ry * sinf(angle);
        vertices[i + 1].color = color;
    }

    // Create indices for triangle fan
    int indices[segments * 3];
    for (int i = 0; i < segments; i++) {
        indices[i * 3] = 0;
        indices[i * 3 + 1] = i + 1;
        indices[i * 3 + 2] = i + 2;
    }

    SDL_RenderGeometry(g_renderer, NULL, vertices, segments + 2, indices, segments * 3);
}

// Draw an ellipse outline (approximation using lines)
inline void draw_ellipse(float cx, float cy, float rx, float ry, const GameColor& color, float thickness) {
    set_draw_color(color);
    const int segments = 32;

    float prevX = cx + rx;
    float prevY = cy;

    for (int i = 1; i <= segments; i++) {
        float angle = (float)i * 2.0f * M_PI / (float)segments;
        float x = cx + rx * cosf(angle);
        float y = cy + ry * sinf(angle);
        SDL_RenderLine(g_renderer, prevX, prevY, x, y);
        prevX = x;
        prevY = y;
    }
    (void)thickness; // TODO: implement thick outlines if needed
}

// Draw a rounded rectangle outline
inline void draw_rounded_rect(float x1, float y1, float x2, float y2, float rx, float ry, const GameColor& color, float thickness) {
    set_draw_color(color);

    // For simplicity, draw a regular rectangle outline
    // A full implementation would draw arcs at corners
    float w = x2 - x1;
    float h = y2 - y1;

    // Clamp radius to half the smaller dimension
    rx = fminf(rx, w / 2);
    ry = fminf(ry, h / 2);

    // Draw the four sides (excluding corners)
    SDL_RenderLine(g_renderer, x1 + rx, y1, x2 - rx, y1);         // Top
    SDL_RenderLine(g_renderer, x1 + rx, y2, x2 - rx, y2);         // Bottom
    SDL_RenderLine(g_renderer, x1, y1 + ry, x1, y2 - ry);         // Left
    SDL_RenderLine(g_renderer, x2, y1 + ry, x2, y2 - ry);         // Right

    // Draw corner arcs (approximated with line segments)
    const int arc_segments = 8;
    for (int i = 0; i < arc_segments; i++) {
        float a1 = M_PI + (M_PI / 2) * i / arc_segments;
        float a2 = M_PI + (M_PI / 2) * (i + 1) / arc_segments;

        // Top-left corner
        SDL_RenderLine(g_renderer,
            x1 + rx + rx * cosf(a1), y1 + ry + ry * sinf(a1),
            x1 + rx + rx * cosf(a2), y1 + ry + ry * sinf(a2));

        // Top-right corner
        SDL_RenderLine(g_renderer,
            x2 - rx + rx * cosf(a1 + M_PI / 2), y1 + ry + ry * sinf(a1 + M_PI / 2),
            x2 - rx + rx * cosf(a2 + M_PI / 2), y1 + ry + ry * sinf(a2 + M_PI / 2));

        // Bottom-right corner
        SDL_RenderLine(g_renderer,
            x2 - rx + rx * cosf(a1 - M_PI), y2 - ry + ry * sinf(a1 - M_PI),
            x2 - rx + rx * cosf(a2 - M_PI), y2 - ry + ry * sinf(a2 - M_PI));

        // Bottom-left corner
        SDL_RenderLine(g_renderer,
            x1 + rx + rx * cosf(a1 - M_PI / 2), y2 - ry + ry * sinf(a1 - M_PI / 2),
            x1 + rx + rx * cosf(a2 - M_PI / 2), y2 - ry + ry * sinf(a2 - M_PI / 2));
    }

    (void)thickness; // TODO: implement thick outlines if needed
}

// Draw a filled rounded rectangle
inline void draw_filled_rounded_rect(float x1, float y1, float x2, float y2, float rx, float ry, const GameColor& color) {
    set_draw_color(color);

    float w = x2 - x1;
    float h = y2 - y1;

    // Clamp radius to half the smaller dimension
    rx = fminf(rx, w / 2);
    ry = fminf(ry, h / 2);

    // Draw center rectangle
    SDL_FRect center = {x1 + rx, y1, w - 2 * rx, h};
    SDL_RenderFillRect(g_renderer, &center);

    // Draw left rectangle
    SDL_FRect left = {x1, y1 + ry, rx, h - 2 * ry};
    SDL_RenderFillRect(g_renderer, &left);

    // Draw right rectangle
    SDL_FRect right = {x2 - rx, y1 + ry, rx, h - 2 * ry};
    SDL_RenderFillRect(g_renderer, &right);

    // Draw corner circles (filled)
    // Top-left
    draw_filled_ellipse(x1 + rx, y1 + ry, rx, ry, color);
    // Top-right
    draw_filled_ellipse(x2 - rx, y1 + ry, rx, ry, color);
    // Bottom-left
    draw_filled_ellipse(x1 + rx, y2 - ry, rx, ry, color);
    // Bottom-right
    draw_filled_ellipse(x2 - rx, y2 - ry, rx, ry, color);
}

#endif // _SDL_COMPAT_H_
