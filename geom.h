#ifndef _GEOM_H_
#define _GEOM_H_

class Point{
    public:
    Point():x(0), y(0){};
    Point(float x, float y):x(x), y(y){}
    ~Point(){};

    float x, y;
};

class Line{
    public:
    Line(){};
    Line(float xa, float ya, float xb, float yb){
        a.x=xa;
        a.y=ya;
        b.x=xb;
        b.y=yb;
    }
    ~Line(){};


    Point a, b;
};

class Rect{
    public:
    Rect(){};
    Rect(float tlx, float tly, float brx, float bry){
        tl.x=tlx;
        tl.y=tly;
        br.x=brx;
        br.y=bry;
    }
    ~Rect(){};


    Point tl, br;
};





#endif