
#ifndef _DUDER_H_
#define _DUDER_H_

#include <allegro5/allegro5.h>
#include "geom.h"
#include "object.h"

using namespace std;

class Duder : public Object{

    public:
    Duder(){};
    Duder(float x, float y, float width, float height);
    ~Duder(){};

    void update(void);
    void draw(void);

    bool is_killed;
    int random_val;
    pair<string, string> bias;
    ALLEGRO_COLOR color;
    
    private:
    Point vel;
    float thick;
    
};




#endif

