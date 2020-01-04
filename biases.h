#ifndef _BIASES_H_
#define _BIASES_H_

#include <fstream>
#include <map>
#include "defines.h"

using namespace std;

class Biases{

    public:
    Biases(){};
    ~Biases(){};
     
    void load(void);

    map<string, string> biases;
    
};


#endif