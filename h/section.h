
#ifndef _SS_SECTION_H_
#define _SS_SECTION_H_

#include "symbol.h"
#include "asm_declarations.h"


namespace ss {
    class Section: public Symbol {
    public:
        Section() : Symbol() {}
        Section(size_t sectionSize, Access access) : sectionSize(sectionSize), access(access), Symbol() {}
        Section(size_t sectionSize, Access access, const std::string& name, SectionType section, unsigned int offset, bool local) 
        : Symbol(name, section, offset, local), sectionSize(sectionSize), access(access) {} 
    
        size_t getSectionSize() const  {
            return sectionSize;
        }
        
        void setSectionSize(size_t size) {
            this->sectionSize = size;
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
