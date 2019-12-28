
#include <iostream>
#include "object.h"
#include "collision.h"

using namespace std;

Object::Object(float x, float y, float width, float height)
:width(width), height(height){
    pos.x = x;
    pos.y = y;
    width2 = width/2;
    height2 = height/2;
    rect = computeRect();
}

Collision Object::collides(Object *obj){
    Rect *objRect = &obj->rect;
    Rect *thisRect = &this->rect;
    bool left = thisRect->br.x >= objRect->tl.x;
    bool right = thisRect->tl.x <= objRect->br.x;
    bool bottom = thisRect->br.y >= objRect->tl.y;
    bool top = thisRect->tl.y <= objRect->br.y;
    
    Collision collision(left, right, top, bottom);
    //cout << left << " " << right << " " << bottom << " " << top << endl;
    //cout << collision.collides << endl;
    return collision;  
}
