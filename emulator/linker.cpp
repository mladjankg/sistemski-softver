#include <map>
#include <vector>
#include <fstream>
#include <algorithm>

#include "elf.h"
#include "linker.h"
#include "relocation.h"
#include "ss_exceptions.h"
#include "asm_declarations.h"
#include "string_tokenizer.h"
#include "linking_file_data.h"

using namespace ss;

bool compareEntry(LinkingFileData* l1, LinkingFileData* l2) {
    return l1->header.entry < l2->header.entry;
}

void Linker::linkFiles(std::vector<std::string>& files) {
    if (files.size() == 0) {
        throw LinkingException("No input files");
    }
    std::vector<LinkingFileData*> parsedFiles;

    LinkingFileData merged;

    for (int i = 0; i < files.size(); i++) {
        LinkingFileData* lfd = this->parseFile(files[i]);
        parsedFiles.push_back(lfd);
    }

    bool startFound = false;
    int startFile = 0;

    //Mapping symbols for each file to it's name and looking for start symbol.
    for(int i = 0; i < parsedFiles.size(); ++i) {
        for(int j = 0; j < parsedFiles[i]->symbolTable.size(); ++j) {
            parsedFiles[i]->symbolMap[parsedFiles[i]->strTab[parsedFiles[i]->symbolTable[j].name]] = &parsedFiles[i]->symbolTable[j];
            if (parsedFiles[i]->strTab[parsedFiles[i]->symbolTable[j].name].compare("START")) {
                if (startFound) {
                    throw LinkingException("Multiple START labels found in files " 
                                           + parsedFiles[startFile]->fileName + " "
                                           + parsedFiles[i]->fileName);
                }
                else {
                    startFound = true;
                    startFile = i;
                    parsedFiles[i]->containsStart = true;
                }
            }
        }
    }

    std::sort(parsedFiles.begin(), parsedFiles.end(), compareEntry);

    size_t currentOffset = 0;
    for (int i = 0; i < parsedFiles.size(); ++i) {
        //Check if file overlaps with another file.
        if (parsedFiles[i]->header.entry < currentOffset) {
            throw AssemblingException("Entry address of file " + parsedFiles[i]->fileName 
                                      + " overlaps with " + (i == 0 ? "IV table" : "file " + parsedFiles[i - 1]->fileName));
        }

        //Check if file exceeds memory limit.
        if (parsedFiles[i]->header.entry + parsedFiles[i]->content[0].size > (ElfWord)MAX_SHORT) {
            throw LinkingException("Content of file " + parsedFiles[i]->fileName + " exceeds memory size");
        }

        merged.content.push_back(parsedFiles[i]->content[0]);
        merged.cumulativeSize += parsedFiles[i]->content[0].size;

        for(int j = 0; j < parsedFiles[i]->symbolTable.size(); ++j) {
            
        }
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

LinkingFileData* Linker::parseFile(const std::string& file) {

    std::ifstream input(file, std::ifstream::in | std::ifstream::binary);

    if (!input.is_open()) {
        throw LinkingException("Cannot open file " + file);
    }

    LinkingFileData* lf = new LinkingFileData();

    StringTokenizer st("/");

    while(st.hasNext()) {
        std::string &str = st.nextToken();
        if (str.compare("") != 0)
            lf->fileName = str;
    }

    //Reading elf header
    input.read((char*)&lf->header, sizeof(ELFHeader));

    if (lf->header.shNum > EXTENDED_SECTION_NUMBER) {
        throw LinkingException("File " + file + " is corrupted, number of section headers exceeds maximum number of section headers.");
    }

    //Positioning file cursor to section headers
    input.seekg(lf->header.shOff, std::ios_base::beg);
    SectionHeader* shd = new SectionHeader[lf->header.shNum];
    
    input.read((char*)&shd[0], lf->header.shNum * sizeof(SectionHeader));
    for (int i = 0; i < lf->header.shNum; ++i) {
        lf->secHeaders.push_back(shd[i]);
    }

    delete[] shd;

    input.clear();
    

    //Reading data rodata text and bss content from file.
    size_t contentBegin = 0;
    size_t contentSize = 0;
    int k = 0;
    for(k = 0; k < lf->header.shNum; ++k) {
        if ((lf->secHeaders[k].type == SectionType::TEXT) ||
            (lf->secHeaders[k].type == SectionType::DATA) ||
            (lf->secHeaders[k].type == SectionType::RO_DATA) ||
            (lf->secHeaders[k].type == SectionType::BSS)) {
            if (contentBegin == 0) {
                contentBegin = lf->secHeaders[k].offset;
            }

            contentSize += lf->secHeaders[k].size;
        }
        else {
            break;
        }
    }
    input.seekg(contentBegin, std::ios_base::beg);
    SectionContent content;
    content.content = new char[contentSize];
    input.read(content.content, contentSize);
    content.size = contentSize;
    lf->content.push_back(content);
    input.clear();

    //Reading relocation tables, symbol table and string table
    for(; k < lf->header.shNum; ++k) {
        switch(lf->secHeaders[k].type) {
            case SectionType::REL_DATA: {
                this->readRelocationTable(input, lf->secHeaders[k], lf->relData);
                break;
            }
            case SectionType::REL_RODATA : {
                this->readRelocationTable(input, lf->secHeaders[k], lf->relRoData);
                break;
            }
            case SectionType::REL_TEXT : {
                this->readRelocationTable(input, lf->secHeaders[k], lf->relText);
                break;
            }
            case SectionType::STR_TAB : {
                input.seekg(lf->secHeaders[k].offset, std::ios_base::beg);
                while (input.tellg() < (lf->secHeaders[k].offset + lf->secHeaders[k].size)) {
                    int size = 0;
                    input.read((char*)&size, sizeof(int));

                    char* a = new char[size + 1];
                    input.read(a, size);
                    a[size] = '\0';
                    std::string str(a);
                    lf->strTab.push_back(str);
                    delete[] a;
                }
                input.clear();
                break;

            }
            case SectionType::SYMB_TAB : {
                input.seekg(lf->secHeaders[k].offset, std::ios_base::beg);
    
                SymTabEntry *st = new SymTabEntry[lf->secHeaders[k].size / lf->secHeaders[k].entSize];
                input.read((char*)st, lf->secHeaders[k].size);
                for (int i = 0; i < lf->secHeaders[k].size / lf->secHeaders[k].entSize; ++i) {
                    lf->symbolTable.push_back(st[i]);
                }
                delete[] st;
                input.clear();
                break;
            }
            default:
                throw LinkingException("Unknown section, sectionCode = " + lf->secHeaders[k].type);
                break;
        }
    }
}

void Linker::readRelocationTable(std::ifstream& input, SectionHeader& header, std::vector<Relocation>& storage) {
    input.seekg(header.offset, std::ios_base::beg);
    
    Relocation* rt = new Relocation[header.size / header.entSize];
    input.read((char*)rt, header.size);
    
    for (int i = 0; i < header.size / header.entSize; ++i) {
        storage.push_back(rt[i]);
    }
    input.clear();
    delete[] rt;
}