
#ifndef _SS_SECTION_H_
#define _SS_SECTION_H_

#include "symbol.h"
#include "asm_declarations.h"


namespace ss {
    class Section: public Symbol {
    public:
        Section() : Symbol() {}
        Section(size_t sectionSize, Access access, unsigned short align = 0) : Symbol(), sectionSize(sectionSize), access(access), align(align), nParsed(0) {}
        Section(size_t sectionSize, Access access, const std::string& name, SectionType section, unsigned int offset, bool local, unsigned short align = 0) 
        : Symbol(name, section, offset, local), sectionSize(sectionSize), access(access), align(align), nParsed(0) {} 
    
        size_t getSectionSize() const  {
            return sectionSize;
        }
        
        void setSectionSize(size_t size) {
            this->sectionSize = size;
        } 

        Access getAccessRights() const {
            return access;
        }

        unsigned short getAlign() const {
            return this->align;
        }

        void setAlign(unsigned short align) {
            this->align = align;
        }
        void increaseParsed() {
            ++this->nParsed;
        }

        unsigned int getNParsed() const {
            return this->nParsed;
        }
    private:
        size_t sectionSize;
        Access access;
        unsigned short align;
        unsigned int nParsed;
    };
};
#endif
