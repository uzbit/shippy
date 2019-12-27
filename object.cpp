

#include "object.h"

Object::Object(float x, float y, float width, float height)
:width(width), height(height){
    pos.x = x;
    pos.y = y;
    width2 = width/2;
    height2 = height/2;
}

