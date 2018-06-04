
#ifndef _SS_SECTION_H_
#define _SS_SECTION_H_

#include "symbol.h"
#include "asm_declarations.h"


namespace ss {
    class Section: public Symbol {
    public:
        Section() : Symbol() {}
        Section(size_t sectionSize, Access access) : sectionSize(sectionSize), access(access), Symbol() {}
        Section(size_t sectionSize, Access access, std::string& name, std::string& section, unsigned int offset, bool local) 
        : sectionSize(sectionSize), access(access), Symbol(name, section, offset, local) {} 
    
        size_t getSectionSize() const  {
            return sectionSize;
        }
        
        Access getAccessRights() const {
            return access;
        }
    private:
        size_t sectionSize;
        Access access;
    };
};
#endif
