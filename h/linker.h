#ifndef _SS_LINKER_H
#define _SS_LINKER_H
#include <vector>
#include <string>

namespace ss { 
    class Linker {
    public:
        void linkFiles(std::vector<std::string>&);
        
        void linkFiles(const char* files[], int num);
        
        //This method parses one binary file and returns it's ELF format representation.
        void parseFile(const std::string&);
    };
}

#endif