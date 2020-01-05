
#include <iostream>
#include "biases.h"


void Biases::load(void){
    int pos;
    ifstream file;
    string line, key, value;
    file.open(BIASES_FILE, ios::in);
    if (file.is_open()){
        while (getline(file, line)){
            if (line.size() <= 1) continue;
            pos = line.find('|');
            key = line.substr(0, pos);
            key = key.substr(0, key.find_last_not_of(" ") + 1);

            value = line.substr(pos + 1, line.size());
            //value = value.substr(value.find_first_not_of(" "), value.size());

            biases[key] = value;
        }
        file.close();
    }    
}