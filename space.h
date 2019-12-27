#ifndef _SPACE_H_
#define _SPACE_H_

#include "body.h"

class Space {
    public:
    Space(){};
    Space(int body_count, int coordx, int coordy);
    ~Space(){};

    void init(void);
    void draw(void);

    private:
    Body **bodies;
    int body_count;
    int coordx, coordy;

};



#endif