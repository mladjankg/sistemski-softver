#ifndef _SS_LINKING_FILE_DATA_
#define _SS_LINKING_FILE_DATA_

#include "elf.h"
#include <vector>
#include <map>
#include "relocation.h"
namespace ss {
    
    class SectionContent {
    public:
        char* content = nullptr;
        size_t size = 0;
        size_t startAddr = 0;


        SectionContent() {}

        ~SectionContent() {
            //delete[] content;
        }
    private:

        void copy(SectionContent& c) {
            this->content = c.content;
            this->size = c.size;
            this->startAddr = c.startAddr;
        }
    };

    class LinkingFileData {
    public:
        LinkingFileData() :  containsStart(false){}

        // bool operator< (const LinkingFileData& lf) const {
        //     return header.entry < lf.header.entry;
        // }
        
        ELFHeader header;
        std::vector<SectionHeader> secHeaders;
        std::vector<Relocation> relText;
        std::vector<Relocation> relData;
        std::vector<Relocation> relRoData;
        std::vector<std::string> strTab;
        std::vector<SymTabEntry> symbolTable;
        std::vector<SectionContent> content;

        std::map<std::string, SymTabEntry*> symbolMap;
        
        std::string fileName;
        bool containsStart;
        size_t cumulativeSize = 0;

        ~LinkingFileData() {
            for(int i = 0; i < content.size(); i++) {
                delete[] content[i].content;
                content[i].content = nullptr;
            }
        }
    };
}
#endif