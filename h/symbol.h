#include <string>

namespace ss {

    class Symbol {
    public:
        Symbol() : no(noCounter++) {

        }

        Symbol() : no(noCounter++) {

        }

        std::string getLabel() {
            return label;
        }

        void setLabel(std::string label) {
            this->label = label; 
        }

        std::string getSection() {
            return section;
        }

        void setLabel(std::string section) {
            this->section = section;
        }

        std::string getOffset() {
            return offset;
        }

        void setOffset(std::string offset) {
            this->offset = offset;
        }

        bool isLocal() {
            return local;
        }

        void setLocal(bool local) {
            this->local = local;
        }

        unsigned int getNo() {
            return no;
        }
    private:
        std::string label;
        std::string section;
        std::string offset;
        bool local;
        unsigned int no;

        static unsigned int noCounter;
    };
}