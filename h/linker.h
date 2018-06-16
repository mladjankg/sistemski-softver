#ifndef _SS_LINKER_H
#define _SS_LINKER_H
#include <vector>
#include <string>
#include "linking_file_data.h"
#include "elf.h"
#include "executable.h"

namespace ss {
    class Relocation;
    class Linker {
    public:
        Linker();
        Executable* linkFiles(std::vector<std::string>&);
        
        Executable* linkFiles(const char* files[], int num);
        
    private:
        void readRelocationTable(std::ifstream&, SectionHeader&, std::vector<Relocation>&);

        //This method parses one binary file and returns it's ELF format representation.
        LinkingFileData* parseFile(const std::string&);
    
        void resolveSectionSymbols(char* mergedContent, std::vector<Relocation>& rel, LinkingFileData* file, SectionType section);
     
        SymTabEntry* findSymbolById(int id, LinkingFileData* file);

        LinkingFileData merged;

        std::vector<LinkingFileData*> parsedFiles;
    };
}

#endif