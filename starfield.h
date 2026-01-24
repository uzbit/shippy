
#ifndef _STARFIELD_H_
#define _STARFIELD_H_

#include <vector>
#include "sdl_compat.h"
#include "geom.h"
#include "object.h"

using namespace std;

class Star : public Object {

    public:
    Star(float x, float y, int lifespan);
    ~Star(){};

    void update(int w, int h);
    void draw(void);

    GameColor color;
    int lifespan, counter;

};

class Starfield{

    public:
    Starfield(){};
    ~Starfield(){};

    void init(int w, int h);
    void update(void);
    void draw(void);

    private:
    int num_stars;
    vector<Star> stars;
    int window_width, window_height;
};




#endif

