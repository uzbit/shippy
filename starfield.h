
#ifndef _STARFIELD_H_
#define _STARFIELD_H_

#include <vector>
#include "sdl_compat.h"
#include "geom.h"
#include "object.h"

using namespace std;

struct Star {
    float x, y;          // world position
    float size;           // radius 0.5-3.0
    float brightness;     // base brightness 0-1
    float twinkle_speed;  // how fast it twinkles
    float twinkle_phase;  // offset so stars don't sync
    float depth;          // 0.0 = far (slow), 1.0 = near (fast parallax)
    GameColor color;
};

struct Nebula {
    float x, y;           // world position
    float radius;         // cloud size
    GameColor color;      // base color (low alpha)
    float depth;          // parallax depth
};

class Starfield {
    public:
    Starfield(){};
    ~Starfield(){};

    void init(int w, int h);
    void update(void);
    void draw(void);

    private:
    int num_stars;
    vector<Star> stars;
    vector<Nebula> nebulas;
    int window_width, window_height;
};

#endif
