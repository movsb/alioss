#include <cstdio>
#include <string>

namespace alioss{

std::string gmt_time();

class progress {
public:
    void set(int num) {
        if(num < 0) num = 0;
        if(num > 100) num = 100;
        printf("\b\b\b\b%3d%%", num);
    }

    void start() {
        printf("    ");
    }

    void end() {
        printf("\b\b\b\b");
    }
};


}

