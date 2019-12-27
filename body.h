#ifndef _BODY_H_
#define _BODY_H_

#include <allegro5/allegro5.h>
#include "geom.h"
#include "defines.h"
#include "object.h"

class Body : public Object{

    public:
    Body(){}
    Body(float x, float y, float width, float height, ALLEGRO_COLOR color, bool filled);
    ~Body(){}
    
    void draw(void);

    bool filled;
    Rect rect;
    ALLEGRO_COLOR color;
};
    


#endif
