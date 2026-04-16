
#include <sstream>
#include "biases.h"
#include "platform.h"


void Biases::load(void){
    std::string content = read_text_asset(BIASES_FILE);
    std::istringstream stream(content);
    std::string line, key, value;
    int pos;

    while (std::getline(stream, line)){
        if (line.size() <= 1) continue;
        pos = line.find('|');
        key = line.substr(0, pos);
        key = key.substr(0, key.find_last_not_of(" ") + 1);
        value = line.substr(pos + 1, line.size());
        biases[key] = value;
    }
}
