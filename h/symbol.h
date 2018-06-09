#ifndef _SS_SYMBOL_H_
#define _SS_SYMBOL_H_

#include <string>
#include "asm_declarations.h"
namespace ss {

    class Section;
    class Symbol {
    public:
        Symbol() : no(noCounter++) {

        }

        Symbol(const std::string& label, SectionType section, unsigned int offset, bool local) 
            : no(noCounter++), name(label), section(section), offset(offset), local(local) {

        }

        const std::string& getName() const {
            return name;
        }

        void setName(std::string& name) {
            this->name = name; 
        }

        const SectionType getSectionCode() const {
            return section;
        }

        void setSectionCode(SectionType section) {
            this->section = section;
        }

        const unsigned int getOffset() const {
            return offset;
        }

        void setOffset(unsigned int offset) {
            this->offset = offset;
        }

        const bool isLocal() const {
            return local;
        }

        void setLocal(bool local) {
            this->local = local;
        }

        const unsigned int getNo() const {
            return no;
        }

        Section* getSectionPtr() const {
            return this->mySection;
        }

        void setSectionPtr(Section* section) { this->mySection = section; }
    private:
        std::string name;      //Label name

        SectionType section;    //Section name
        Section* mySection = nullptr;     //Section that this symbol belongs
        unsigned int offset;     //Offset from start of the section

        bool local;             //Is label local or global

        unsigned int no;        //Symbol identifier

        static unsigned int noCounter;
    };
}

#endif