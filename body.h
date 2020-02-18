#ifndef _BODY_H_
#define _BODY_H_

#include <allegro5/allegro5.h>
#include "object.h"
#include "collision.h"

class Body : public Object{

    public:
    Body(){}
    Body(float x, float y, float width, float height, ALLEGRO_COLOR color, bool filled);
    ~Body(){}
    
    void draw(void);

    bool filled;
    int round, thick;
    ALLEGRO_COLOR color;
    float density;
};
    


#endif
