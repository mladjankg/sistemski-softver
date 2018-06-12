#ifndef _SS_RELOCATION_H_
#define _SS_RELOCATION_H_

#include "asm_declarations.h"

namespace ss {
    struct Relocation {
    public:
        Relocation() {}
        Relocation(short offset, RelocationType type, unsigned int id) 
            : offset(offset), type(type), id(id) {} 

        std::string toString() const;
            
        short offset;
        RelocationType type;
        unsigned int id;
    };
}
#endif