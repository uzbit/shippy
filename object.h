#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "geom.h"
#include "collision.h"

class Object{
    public:
    Object(){}
    Object(float x, float y, float width, float height);
    ~Object(){}

    Collision collides(Object *obj);

    Rect computeRect(void){
        rect = Rect(
            pos.x-width2, pos.y-height2, 
            pos.x+width2, pos.y+height2
        );
        return rect;
    }


    Point pos;
    Rect rect;
    float width, height;
    float width2, height2;
    Collision prev_collision;
};


#endif