#ifndef _SS_EXECUTABLE_H_
#define _SS_EXECUTABLE_H_
#include <vector>

struct Limit {
    unsigned short high;
    unsigned short low;

    bool operator< (const Limit& l) {
        return low < l.low;
    }
};

struct Executable {
    char* content;
    unsigned short startAddress;
    std::vector<Limit> ex;
    std::vector<Limit> rw;
    std::vector<Limit> rd;
};

#endif