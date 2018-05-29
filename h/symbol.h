#include <string>

namespace ss {

    class Symbol {
    public:
        Symbol() : no(noCounter++) {

        }

        Symbol(std::string& label, std::string& section, unsigned int offset, bool local) 
            : no(noCounter++), label(label), section(section), offset(offset), local(local) {

        }

        const std::string getLabel() const {
            return label;
        }

        void setLabel(std::string& label) {
            this->label = label; 
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
        std::string label;      //Label name
        std::string section;    //Section name
        unsigned int offset;     //Offset from start of the section
        bool local;             //Is label local or global
        unsigned int no;        //Symbol identifier

        static unsigned int noCounter;
    };
}