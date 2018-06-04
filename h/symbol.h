#ifndef _SS_SYMBOL_H_
#define _SS_SYMBOL_H_

#include <string>

namespace ss {

    class Symbol {
    public:
        Symbol() : no(noCounter++) {

        }

        Symbol(std::string& label, std::string& section, unsigned int offset, bool local) 
            : no(noCounter++), name(label), section(section), offset(offset), local(local) {

        }

        const std::string getName() const {
            return name;
        }

        void setName(std::string& name) {
            this->name = name; 
        }

        const std::string& getSection() const {
            return section;
        }

        void setSection(std::string& section) {
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
    private:
        std::string name;      //Label name
        std::string section;    //Section name
        unsigned int offset;     //Offset from start of the section
        bool local;             //Is label local or global
        unsigned int no;        //Symbol identifier

        static unsigned int noCounter;
    };
}

#endif