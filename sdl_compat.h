#ifndef _SDL_COMPAT_H_
#define _SDL_COMPAT_H_

#include <SDL3/SDL.h>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Forward declare ColorTheme enum
enum class ColorTheme;

// Type alias for color - SDL3 uses SDL_FColor for floating-point colors
typedef SDL_FColor GameColor;

// Global renderer pointer (set during init)
extern SDL_Renderer* g_renderer;

// Global trippy/theme state (set by game based on current space)
extern float g_hueShift;      // Current hue shift in degrees (0-360)
extern int g_trippyLevel;     // 0=none, 1+=intensity of color cycling
extern int g_colorTheme;      // ColorTheme as int for cross-file compatibility
extern int g_tracerLength;    // 0=none, 1-10 = trail length

// RGB to HSV conversion
inline void rgb_to_hsv(float r, float g, float b, float& h, float& s, float& v) {
    float maxc = std::max({r, g, b});
    float minc = std::min({r, g, b});
    v = maxc;
    if (minc == maxc) {
        h = 0.0f;
        s = 0.0f;
        return;
    }
    s = (maxc - minc) / maxc;
    float rc = (maxc - r) / (maxc - minc);
    float gc = (maxc - g) / (maxc - minc);
    float bc = (maxc - b) / (maxc - minc);
    if (r == maxc)
        h = bc - gc;
    else if (g == maxc)
        h = 2.0f + rc - bc;
    else
        h = 4.0f + gc - rc;
    h = fmodf(h / 6.0f, 1.0f);
    if (h < 0.0f) h += 1.0f;
}

// HSV to RGB conversion
inline void hsv_to_rgb(float h, float s, float v, float& r, float& g, float& b) {
    if (s == 0.0f) {
        r = g = b = v;
        return;
    }
    int i = (int)(h * 6.0f);
    float f = (h * 6.0f) - i;
    float p = v * (1.0f - s);
    float q = v * (1.0f - s * f);
    float t = v * (1.0f - s * (1.0f - f));
    i = i % 6;
    switch (i) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
        default: r = g = b = 0; break;
    }
}

// Rotate hue by degrees
inline GameColor rotate_hue(const GameColor& color, float degrees) {
    float h, s, v;
    rgb_to_hsv(color.r, color.g, color.b, h, s, v);
    h += degrees / 360.0f;
    h = fmodf(h, 1.0f);
    if (h < 0.0f) h += 1.0f;
    float r, g, b;
    hsv_to_rgb(h, s, v, r, g, b);
    return SDL_FColor{r, g, b, color.a};
}

// Apply color theme transformation
inline GameColor apply_theme(const GameColor& color, int theme) {
    float h, s, v;
    rgb_to_hsv(color.r, color.g, color.b, h, s, v);

    switch (theme) {
        case 0: // NORMAL - no change
            break;
        case 1: // WARM - shift toward red/orange (0-60 degrees)
            h = fmodf(h * 0.17f, 1.0f);  // Compress hue to red-orange range
            s = std::min(1.0f, s * 1.2f);
            break;
        case 2: // COOL - shift toward blue/purple (180-300 degrees)
            h = 0.5f + fmodf(h * 0.33f, 0.33f);  // Map to blue-purple range
            s = std::min(1.0f, s * 1.1f);
            break;
        case 3: // NEON - maximize saturation
            s = std::min(1.0f, s + 0.5f);
            v = std::min(1.0f, v + 0.3f);
            break;
        case 4: // MONO - grayscale with slight tint
            s *= 0.15f;
            break;
    }

    float r, g, b;
    hsv_to_rgb(h, s, v, r, g, b);
    return SDL_FColor{r, g, b, color.a};
}

// Helper function to create a color from RGB values (0-255)
inline GameColor map_rgb(int r, int g, int b) {
    return SDL_FColor{r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
}

// Helper function to create a color from RGB float values (0.0-1.0)
inline GameColor map_rgb_f(float r, float g, float b) {
    return SDL_FColor{r, g, b, 1.0f};
}

// Transform a color by applying theme and trippy effects
inline GameColor transform_color(const GameColor& color) {
    GameColor finalColor = color;

    // Apply theme transformation
    if (g_colorTheme != 0) {
        finalColor = apply_theme(finalColor, g_colorTheme);
    }

    // Apply trippy hue shift
    if (g_trippyLevel > 0 && g_hueShift != 0.0f) {
        finalColor = rotate_hue(finalColor, g_hueShift);
    }

    return finalColor;
}

// Set the current draw color on the renderer (applies theme and trippy effect)
inline void set_draw_color(const GameColor& color) {
    GameColor finalColor = transform_color(color);
    SDL_SetRenderDrawColorFloat(g_renderer, finalColor.r, finalColor.g, finalColor.b, finalColor.a);
}

// Draw a filled rectangle
inline void draw_filled_rect(float x1, float y1, float x2, float y2, const GameColor& color) {
    set_draw_color(color);
    SDL_FRect rect = {x1, y1, x2 - x1, y2 - y1};
    SDL_RenderFillRect(g_renderer, &rect);
}

// Draw a line with thickness using a quad
inline void draw_line(float x1, float y1, float x2, float y2, const GameColor& color, float thickness) {
    // Calculate direction vector
    float dx = x2 - x1;
    float dy = y2 - y1;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.0001f) return;

    // Perpendicular unit vector
    float px = -dy / len;
    float py = dx / len;

    // Half thickness offset
    float hx = px * thickness * 0.5f;
    float hy = py * thickness * 0.5f;

    // Transform color for theme/trippy effects
    GameColor tc = transform_color(color);

    // Create 4 vertices forming a quad
    SDL_Vertex vertices[4];
    vertices[0].position.x = x1 + hx;
    vertices[0].position.y = y1 + hy;
    vertices[0].color = tc;

    vertices[1].position.x = x1 - hx;
    vertices[1].position.y = y1 - hy;
    vertices[1].color = tc;

    vertices[2].position.x = x2 - hx;
    vertices[2].position.y = y2 - hy;
    vertices[2].color = tc;

    vertices[3].position.x = x2 + hx;
    vertices[3].position.y = y2 + hy;
    vertices[3].color = tc;

    // Two triangles: 0-1-2 and 0-2-3
    int indices[6] = {0, 1, 2, 0, 2, 3};

    SDL_RenderGeometry(g_renderer, NULL, vertices, 4, indices, 6);
}

// Draw a filled triangle using SDL_RenderGeometry
inline void draw_filled_triangle(float x1, float y1, float x2, float y2, float x3, float y3, const GameColor& color) {
    GameColor tc = transform_color(color);
    SDL_Vertex vertices[3];

    vertices[0].position.x = x1;
    vertices[0].position.y = y1;
    vertices[0].color = tc;

    vertices[1].position.x = x2;
    vertices[1].position.y = y2;
    vertices[1].color = tc;

    vertices[2].position.x = x3;
    vertices[2].position.y = y3;
    vertices[2].color = tc;

    SDL_RenderGeometry(g_renderer, NULL, vertices, 3, NULL, 0);
}

// Draw a filled ellipse (approximation using triangles)
inline void draw_filled_ellipse(float cx, float cy, float rx, float ry, const GameColor& color) {
    GameColor tc = transform_color(color);
    const int segments = 32;
    SDL_Vertex vertices[segments + 2];

    // Center vertex
    vertices[0].position.x = cx;
    vertices[0].position.y = cy;
    vertices[0].color = tc;

    // Circle vertices
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i * 2.0f * M_PI / (float)segments;
        vertices[i + 1].position.x = cx + rx * cosf(angle);
        vertices[i + 1].position.y = cy + ry * sinf(angle);
        vertices[i + 1].color = tc;
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

// Draw an ellipse outline as a thick ring
inline void draw_ellipse(float cx, float cy, float rx, float ry, const GameColor& color, float thickness) {
    GameColor tc = transform_color(color);
    const int segments = 32;

    // Inner and outer radii
    float half_t = thickness * 0.5f;
    float rx_outer = rx + half_t;
    float ry_outer = ry + half_t;
    float rx_inner = rx - half_t;
    float ry_inner = ry - half_t;

    // Clamp inner radii to avoid negative values
    if (rx_inner < 0) rx_inner = 0;
    if (ry_inner < 0) ry_inner = 0;

    // Generate vertices: alternating outer and inner points
    SDL_Vertex vertices[segments * 2 + 2];
    for (int i = 0; i <= segments; i++) {
        float angle = (float)i * 2.0f * M_PI / (float)segments;
        float cos_a = cosf(angle);
        float sin_a = sinf(angle);

        // Outer vertex
        vertices[i * 2].position.x = cx + rx_outer * cos_a;
        vertices[i * 2].position.y = cy + ry_outer * sin_a;
        vertices[i * 2].color = tc;

        // Inner vertex
        vertices[i * 2 + 1].position.x = cx + rx_inner * cos_a;
        vertices[i * 2 + 1].position.y = cy + ry_inner * sin_a;
        vertices[i * 2 + 1].color = tc;
    }

    // Create indices for triangle strip (2 triangles per segment)
    int indices[segments * 6];
    for (int i = 0; i < segments; i++) {
        int base = i * 2;
        int idx = i * 6;
        // Triangle 1: outer[i], inner[i], outer[i+1]
        indices[idx] = base;
        indices[idx + 1] = base + 1;
        indices[idx + 2] = base + 2;
        // Triangle 2: inner[i], inner[i+1], outer[i+1]
        indices[idx + 3] = base + 1;
        indices[idx + 4] = base + 3;
        indices[idx + 5] = base + 2;
    }

    SDL_RenderGeometry(g_renderer, NULL, vertices, segments * 2 + 2, indices, segments * 6);
}

// Helper function to draw a thick arc (quarter ellipse)
inline void draw_thick_arc(float cx, float cy, float rx, float ry, float start_angle, float end_angle, const GameColor& color, float thickness) {
    GameColor tc = transform_color(color);
    const int arc_segments = 8;

    float half_t = thickness * 0.5f;
    float rx_outer = rx + half_t;
    float ry_outer = ry + half_t;
    float rx_inner = rx - half_t;
    float ry_inner = ry - half_t;

    if (rx_inner < 0) rx_inner = 0;
    if (ry_inner < 0) ry_inner = 0;

    SDL_Vertex vertices[(arc_segments + 1) * 2];
    for (int i = 0; i <= arc_segments; i++) {
        float angle = start_angle + (end_angle - start_angle) * (float)i / (float)arc_segments;
        float cos_a = cosf(angle);
        float sin_a = sinf(angle);

        vertices[i * 2].position.x = cx + rx_outer * cos_a;
        vertices[i * 2].position.y = cy + ry_outer * sin_a;
        vertices[i * 2].color = tc;

        vertices[i * 2 + 1].position.x = cx + rx_inner * cos_a;
        vertices[i * 2 + 1].position.y = cy + ry_inner * sin_a;
        vertices[i * 2 + 1].color = tc;
    }

    int indices[arc_segments * 6];
    for (int i = 0; i < arc_segments; i++) {
        int base = i * 2;
        int idx = i * 6;
        indices[idx] = base;
        indices[idx + 1] = base + 1;
        indices[idx + 2] = base + 2;
        indices[idx + 3] = base + 1;
        indices[idx + 4] = base + 3;
        indices[idx + 5] = base + 2;
    }

    SDL_RenderGeometry(g_renderer, NULL, vertices, (arc_segments + 1) * 2, indices, arc_segments * 6);
}

// Draw a rounded rectangle outline with thickness
inline void draw_rounded_rect(float x1, float y1, float x2, float y2, float rx, float ry, const GameColor& color, float thickness) {
    float w = x2 - x1;
    float h = y2 - y1;

    // Clamp radius to half the smaller dimension
    rx = fminf(rx, w / 2);
    ry = fminf(ry, h / 2);

    // Draw the four sides with thickness
    draw_line(x1 + rx, y1, x2 - rx, y1, color, thickness);         // Top
    draw_line(x1 + rx, y2, x2 - rx, y2, color, thickness);         // Bottom
    draw_line(x1, y1 + ry, x1, y2 - ry, color, thickness);         // Left
    draw_line(x2, y1 + ry, x2, y2 - ry, color, thickness);         // Right

    // Draw corner arcs with thickness
    // Top-left: angles from PI to 3*PI/2
    draw_thick_arc(x1 + rx, y1 + ry, rx, ry, M_PI, 3.0f * M_PI / 2.0f, color, thickness);
    // Top-right: angles from 3*PI/2 to 2*PI
    draw_thick_arc(x2 - rx, y1 + ry, rx, ry, 3.0f * M_PI / 2.0f, 2.0f * M_PI, color, thickness);
    // Bottom-right: angles from 0 to PI/2
    draw_thick_arc(x2 - rx, y2 - ry, rx, ry, 0, M_PI / 2.0f, color, thickness);
    // Bottom-left: angles from PI/2 to PI
    draw_thick_arc(x1 + rx, y2 - ry, rx, ry, M_PI / 2.0f, M_PI, color, thickness);
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
