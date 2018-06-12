#include "linker.h"
#include <vector>
#include <fstream>
#include "ss_exceptions.h"
#include "elf.h"
#include "asm_declarations.h"

using namespace ss;

void Linker::linkFiles(std::vector<std::string>& files) {
    if (files.size() == 0) {
        throw LinkingException("No input files");
    }

    for (int i = 0; i < files.size(); i++) {
        this->parseFile(files[i]);

    }
}

void Linker::linkFiles(const char* files[], int num) {
    std::vector<std::string> filesVec;

    if (num <= 0) {
        throw LinkingException("No input files");
    } 
    for (int i = 0; i < num; i++) {
        filesVec.push_back(files[i]);
    }

    this->linkFiles(filesVec);
}

void Linker::parseFile(const std::string& file) {

    std::ifstream input(file, std::ifstream::in | std::ifstream::binary);

    if (!input.is_open()) {
        throw LinkingException("Cannot open file " + file);
    }

    LinkingFile* lf = new LinkingFile();

    //Reading elf header
    
    input.read((char*)&lf.header, sizeof(ELFHeader));

    if (header.shNum > EXTENDED_SECTION_NUMBER) {
        throw LinkingException("File " + file + " is corrupted, number of section headers exceeds maximum number of section headers.");
    }

    //Positioning file cursor to section headers
    input.seekg(header.shOff, std::ios_base::beg);
    SectionHeader* shd = new SectionHeader[header.shNum];
    
    input.read((char*)&shd[0], lf.header.shNum * sizeof(SectionHeader));
    lf.secHeaders = shd;

    input.clear();
    
    for(int i = 0; i < header.shNum)

}