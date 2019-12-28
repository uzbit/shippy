#ifndef _COLLISION_H_
#define _COLLISION_H_

#include <iostream>

using namespace std;

class Collision{

    public:
    Collision(){}
    Collision(bool left, bool right, bool top, bool bottom)
        :left(left), right(right), top(top), bottom(bottom){
        collides = (left && right && bottom && top);
    }
    
    void print(void){
        cout << left << " " << right << " " << top << " " << bottom << endl;
        cout << collides << endl;
    }

    bool left, right, top, bottom;
    bool collides;
};


#endif