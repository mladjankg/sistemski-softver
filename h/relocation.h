#ifndef _SS_RELOCATION_H_
#define _SS_RELOCATION_H_

#include "asm_declarations.h"
#include <iomanip>
#include <string>
#include <sstream>

namespace ss {
    struct Relocation {
    public:
        Relocation(short offset, RelocationType type, unsigned int id) 
            : offset(offset), type(type), id(id) {} 

        std::string toString() const {
            std::string relType = this->type == RelocationType::R_386_PC16 ? "R_386_PC16" :
                                  this->type == RelocationType::R_386_16 ? "R_386_16" :
                                  "R_386_32";

            std::stringstream stream;
            stream << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << std::hex << this->offset 
              << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << relType 
              << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << this->id;

            return stream.str();
        }  
            
        short offset;
        RelocationType type;
        unsigned int id;
    };
}
#endif