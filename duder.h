
#ifndef _DUDER_H_
#define _DUDER_H_

#include <allegro5/allegro5.h>
#include "geom.h"
#include "object.h"


class Duder : public Object{

    public:
    Duder(){};
    Duder(float x, float y, int bias_index);
    ~Duder(){};

    void update(void);
    void draw(void);

    private:
    Point vel;
    int bias_index;
    void draw_flame(ALLEGRO_TRANSFORM *transform);
    void draw_flames(void);
};




#endif

