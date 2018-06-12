#ifndef _SS_LINKING_FILE_DATA_
#define _SS_LINKING_FILE_DATA_

#include "elf.h"
#include <vector>
#include <map>
#include "relocation.h"
namespace ss {
    
    struct SectionContent {
        char* content = nullptr;
        size_t size = 0;

        ~SectionContent() {
            delete[] content;
        }
    };

    struct LinkingFileData {
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

    };
}
#endif