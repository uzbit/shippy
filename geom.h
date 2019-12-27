#ifndef _GEOM_H_
#define _GEOM_H_

class Point{
    public:
    Point(){};
    Point(int x, int y):x(x), y(y){}
    ~Point(){};

    
    int x, y;
};

class Line{
    public:
    Line(){};
    Line(int xa, int ya, int xb, int yb){
        a.x=xa;
        a.y=ya;
        b.x=xb;
        b.y=yb;
    }
    ~Line(){};


    Point a, b;
};

#endif