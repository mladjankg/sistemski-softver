#include <map>
#include <vector>
#include <fstream>
#include <algorithm>
#include <iostream>
#include <iomanip>

#include "elf.h"
#include "linker.h"
#include "executable.h"
#include "relocation.h"
#include "ss_exceptions.h"
#include "asm_declarations.h"
#include "string_tokenizer.h"
#include "linking_file_data.h"
//#define LINKER_OUTPUT
using namespace ss;

// bool compareEntry(LinkingFileData* l1, LinkingFileData* l2) {
//     return l1->header.entry < l2->header.entry;
// }

Linker::Linker() {}

Executable* Linker::linkFiles(std::vector<std::string>& files) {
    if (files.size() == 0) {
        throw LinkingException("No input files");
    }

    for (int i = 0; i < files.size(); i++) {
        LinkingFileData* lfd = this->parseFile(files[i]);
        this->parsedFiles.push_back(lfd);
    }


    bool startFound = false;
    int startFile = 0;
    unsigned short startAddress = 0;
    //Mapping symbols for each file to it's name and looking for start symbol.
    for(int i = 0; i < parsedFiles.size(); ++i) {
        for(int j = 0; j < parsedFiles[i]->symbolTable.size(); ++j) {
            parsedFiles[i]->symbolMap[parsedFiles[i]->strTab[parsedFiles[i]->symbolTable[j].name]] = &parsedFiles[i]->symbolTable[j];
            if (parsedFiles[i]->strTab[parsedFiles[i]->symbolTable[j].name].compare("START") == 0) {
                if (startFound) {
                    throw LinkingException("Multiple START labels found in files " 
                                           + parsedFiles[startFile]->fileName + " "
                                           + parsedFiles[i]->fileName);
                }
                else {
                    startFound = true;
                    startFile = i;
                    parsedFiles[i]->containsStart = true;
                    #ifdef RELATIVE_OFFSET
                    //TODO: Implement this
                    #else
                    startAddress = parsedFiles[i]->symbolTable[j].offset;
                    #endif
                }
            }
        }
    }

    if (!startFound) {
        throw LinkingException("Missing program starting point.");
    }

    for (int i = 0; i < parsedFiles.size() - 1; ++i) {
        for (int j = i + 1; j < parsedFiles.size(); ++j) {
            if (parsedFiles[i]->header.entry > parsedFiles[j]->header.entry) {
                LinkingFileData* lfdpom = parsedFiles[i];
                parsedFiles[i] = parsedFiles[j];
                parsedFiles[j] = lfdpom;
            }
        }
    }
    //std::sort(parsedFiles.begin(), parsedFiles.end(), compareEntry);

    size_t currentOffset = 0;

    for (int i = 0; i < parsedFiles.size(); ++i) {
        //Check if file overlaps with another file.
        if (parsedFiles[i]->header.entry < currentOffset) {
            throw LinkingException("Entry address of file " + parsedFiles[i]->fileName 
                                      + " overlaps with " + (i == 0 ? "IV table" : "file " + parsedFiles[i - 1]->fileName));
        }

        //Check if file exceeds memory limit.
        if (parsedFiles[i]->header.entry + parsedFiles[i]->content[0].size >= STACK_START - STACK_SIZE) {
            throw LinkingException("Content of file " + parsedFiles[i]->fileName + " exceeds allowed memory size");
        }

        merged.content.push_back(parsedFiles[i]->content[0]);
        merged.cumulativeSize += parsedFiles[i]->content[0].size;
        currentOffset = parsedFiles[i]->content[0].size + parsedFiles[i]->header.entry;
        std::vector<SymTabEntry>& symTab = parsedFiles[i]->symbolTable;
        std::vector <std::string>& strTab =  parsedFiles[i]->strTab;
        for(int j = 0; j < symTab.size(); ++j) {
            std::string& name = strTab[symTab[j].name];
            if ((name.compare(".data") == 0) || (name.compare(".text") == 0) || (name.compare(".rodata") == 0) || (name.compare(".bss") == 0)) {
                continue;
            }
            if (symTab[j].section == SectionType::UDF) {
                continue;
            }
            if (merged.symbolMap.find(name) != merged.symbolMap.end()) {
              
                throw LinkingException("Found multiple definitions of symbol " + strTab[symTab[j].name]);
            }
            
            merged.symbolMap[strTab[symTab[j].name]] = &symTab[j];
        }
    }
    char* mergedContent = new char[((unsigned)MAX_SHORT + 1)];

    //merging content
    for (int i = 0; i < merged.content.size(); i++) {
        size_t start = merged.content[i].startAddr;
        for(int j = start; j < start + merged.content[i].size; ++j) {
            mergedContent[j] = merged.content[i].content[j - start];
        }
    }
   


    for (int i = 0; i < parsedFiles.size(); ++i) {
        //LinkingFileData* f = parsedFiles[i];
        this->resolveSectionSymbols(mergedContent, parsedFiles[i]->relData, parsedFiles[i], SectionType::DATA);
        this->resolveSectionSymbols(mergedContent, parsedFiles[i]->relRoData, parsedFiles[i], SectionType::RO_DATA);
        this->resolveSectionSymbols(mergedContent, parsedFiles[i]->relText, parsedFiles[i], SectionType::TEXT);
       
    }

    #ifdef LINKER_OUTPUT
    std::cout<<"\n";

    for (int i = 0; i < parsedFiles.size(); ++i) {
        size_t start = parsedFiles[i]->header.entry;
        size_t size = parsedFiles[i]->content[0].size;
        for (int j = start; j < start + size; j++) {
            std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<((short)mergedContent[j] & 0xFF) << " ";
        }
    }
    std::cout << std::endl <<std::flush;
    #endif
    // for (int i = 0; i < merged.content.size(); i++) {
    //     size_t start = merged.content[i].startAddr;
    //     for(int j = 0; j < merged.content[i].size; ++j) {
    //         std::cout<<std::hex<<std::setfill('0')<<std::setw(2)<<((short)merged.content[i].content[j] & 0xFF) << " ";
    //     }
        
    // }

    //Getting sections and setting their access rights.
    std::cout<<std::flush;
    Executable* e = new Executable();
    for(int i = 0; i < parsedFiles.size(); ++i) {
        LinkingFileData* file = parsedFiles[i];
        for (int j = 0; j < file->secHeaders.size(); ++j) {
            Limit l;
            l.low = file->header.entry + file->secHeaders[j].offset - file->header.ehSize;
            l.high = file->header.entry + file->secHeaders[j].offset - file->header.ehSize + file->secHeaders[j].size - 1;
            if (file->secHeaders[j].type == TEXT) {
                #ifdef LINKER_OUTPUT
                std::cout << "File: " << file->fileName << " section: text"
                        << " lower:" << l.low << " higher:" << l.high << std::endl << std::flush;
                #endif
                e->ex.push_back(l);
            }
            if (file->secHeaders[j].type == DATA) {
                #ifdef LINKER_OUTPUT
                std::cout << "File: " << file->fileName << " section: data"
                        << " lower:" << l.low << " higher: " << l.high << std::endl << std::flush;
                #endif
                e->rw.push_back(l);
            }
            if (file->secHeaders[j].type == RO_DATA) {
                #ifdef LINKER_OUTPUT
                std::cout << "File: " << file->fileName << " section: " << "rodata"
                        << " lower:" << l.low << " higher:" << l.high << std::endl << std::flush;
                #endif
                e->rd.push_back(l);
            }
            if (file->secHeaders[j].type == BSS) {
                #ifdef LINKER_OUTPUT
                std::cout << "File: " << file->fileName << " section: " << "bss"
                        << " lower:" << l.low << " higher:" << l.high << std::endl << std::flush; 
                #endif
                e->rw.push_back(l);
            }

        }
    }

    Limit stack;
    stack.high = STACK_START - 1;
    stack.low = STACK_START - STACK_SIZE;
    Limit io;
    io.high = 0x10000 - 1;
    io.low = IO_RESERVED;
    e->rw.push_back(io);
    e->rw.push_back(stack);
    if (e->ex.size() != 0) {
        std::sort(e->ex.begin(), e->ex.end());
    }
    if (e->rd.size() != 0) {
        std::sort(e->rd.begin(), e->rd.end());
    }
    if (e->rw.size() != 0) {
        std::sort(e->rw.begin(), e->rw.end());
    }
    std::vector<Limit>& ex = e->ex;
    for(int i = 0; i < parsedFiles.size(); ++i) {
        delete parsedFiles[i];
    }


    e->content = mergedContent;
    e->startAddress = startAddress;

    return e;
}

SymTabEntry* Linker::findSymbolById(int id, LinkingFileData* file) {
    std::string& name = file->strTab[file->symbolTable[id].name];

    
    for (int i = 0; i < file->symbolTable.size(); i++) {
        if ((file->symbolTable[i].id == id) && (file->symbolTable[i].section != SectionType::UDF))
            return &file->symbolTable[i];
        
    }

    auto its = merged.symbolMap.find(name);
    if (its != merged.symbolMap.end()) {
        return its->second;
    }
    else {
        return nullptr;
    }
    // for (auto it = merged.symbolMap.begin(); it != merged.symbolMap.end(); ++it)
    //     if (it->second->id == id) 
    //         return it->second;
    

    // return nullptr;
}

void Linker::resolveSectionSymbols(char* mergedContent, std::vector<Relocation>& rel, LinkingFileData* file, SectionType section) {
    if (rel.size() == 0) return;

    SectionHeader* sh = nullptr;

    for (int i = 0; i < file->secHeaders.size(); ++i) {
        if (file->secHeaders[i].type == section) {
            sh = &file->secHeaders[i];
            break;
        }
    }

    if (sh == nullptr) {
        throw LinkingException("Unespected error in method resolveSectionSymbols, section not found.");
    }
    for(int j = 0; j < rel.size(); ++j) {
        Relocation& r = rel[j];
        char* refptr = (mergedContent + r.offset + file->header.entry + sh->offset - file->header.ehSize);
        SymTabEntry* symbol = nullptr;

        symbol = this->findSymbolById(r.id, file);
        
        short oldLow = (*refptr) & 0xFF;
        short oldHigh = *(refptr + 1) & 0xFF;

        ElfWord oldValue = (oldHigh << 8) | oldLow;
        ElfWord newValue = 0;
        if (symbol == nullptr) {
            throw LinkingException("Symbol " + file->strTab[file->symbolTable[r.id].name] + " not defined.");
        }

        if (r.type == RelocationType::R_386_PC16) {
            ElfWord refaddr = r.offset + sh->offset - file->header.ehSize; // + merged.symbolMap[file->strTab[file->symbolTable[r.id].name]]->offset;

            newValue = (ElfWord)(symbol->offset  + oldValue - refaddr - file->header.entry);
        }

        //std::cout << "Relocating symbol " + file->strTab[symbol->name] << " old value :";
        if (r.type == RelocationType::R_386_16) {
            newValue = (ElfWord)(symbol->offset + oldValue);
        }

        char newHigh = (newValue >> 8) & 0xFF;
        char newLow = (newValue) & 0xFF;

        *refptr = newLow;
        *(refptr + 1) = newHigh;
        #ifdef LINKER_OUTPUT
          std::cout << "Relocating symbol " + file->strTab[symbol->name] 
                     << " old value:" << std::hex << (short)oldLow << ' ' <<std::hex << (short)oldHigh
                     << " new value:" << std::hex << (short)newLow << ' ' << std::hex << (short)newHigh << std::endl;
        #endif
   }
}

Executable* Linker::linkFiles(const char* files[], int num) {
    std::vector<std::string> filesVec;

    if (num <= 0) {
        throw LinkingException("No input files");
    } 
    for (int i = 0; i < num; i++) {
        filesVec.push_back(files[i]);
    }

    return this->linkFiles(filesVec);

}

LinkingFileData* Linker::parseFile(const std::string& file) {

    std::ifstream input(file, std::ifstream::in | std::ifstream::binary);

    if (!input.is_open()) {
        throw LinkingException("Cannot open file " + file);
    }

    LinkingFileData* lf = new LinkingFileData();

    StringTokenizer st("/");

    st.tokenize(file);
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
    content.startAddr = lf->header.entry;
    
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

    return lf;
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