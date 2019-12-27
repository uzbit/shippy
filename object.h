#ifndef _OBJECT_H_
#define _OBJECT_H_

#include "geom.h"

class Object{
    public:
    Object(){}
    Object(float x, float y, float width, float height);
    ~Object(){}

    bool collides(Object *obj){
        Rect objRect = obj->computeRect();
        Rect thisRect = this->computeRect();
        return false;
    }
    Rect computeRect(void){
        return Rect(
            pos.x-width2, pos.y-height2, 
            pos.x+width2, pos.y+height2
        );
    }


    Point pos;
    float width, height;
    float width2, height2;

};


#endif