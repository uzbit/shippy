#ifndef _BODY_H_
#define _BODY_H_

#include <allegro5/allegro5.h>
#include "defines.h"
#include "object.h"

class Body : public Object{

    public:
    Body(){}
    Body(float x, float y, float width, float height, ALLEGRO_COLOR color);
    ~Body(){}
    

};

#endif
