#include <string>
#include <fstream>
#include <unordered_map>
#include <iterator>
#include <iostream>

#include "assembler.h"
#include "utils.h"
#include "string_tokenizer.h"
#include "symbol.h"
#include "ss_exceptions.h"
#include "instruction.h"
#include "operand.h"
#include "section.h"

using namespace ss;
using namespace std;
Assembler::Assembler(std::ifstream* in, std::ofstream* out) : input(in), output(out), labelRegex("^[a-zA-Z_]\\w*$") {

}

Assembler::Assembler(Assembler&& a) {
    move(a);
}

void Assembler::move(Assembler& a) {
    this->input = a.input;
    this->output = a.output;
    this->symbolTable = a.symbolTable;
}

Assembler Assembler::getInstance(std::string& inputFile, std::string& outputFile) {
    
    //Creating input stream.
    std::ifstream* in = new std::ifstream(inputFile, std::ifstream::in);

    //Creating output stream.
    std::ofstream* out = new std::ofstream(outputFile, std::ofstream::out | std::ofstream::trunc);
    
    //Checking if output stream was created correctly.
    std::string message = Utils::empty;
    if (!out->is_open()) {
        message += "Cannot open file " + outputFile + ".\n";

        throw FileException(message.c_str());
    }

    //Checking if input stream was created correctly.
    if (!in->is_open()) {
        message += "Cannot open file " + inputFile + ".\n";
        
        out->close();
        delete out;
  
        throw FileException(message.c_str());
    }
    
    return Assembler(in, out);
}

void Assembler::assemble() {
    this->firstPass();
}
    
void Assembler::firstPass() {
    std::string line;
    
    
    int locationCounter = 0;
    int lineNumber = 0;
    
    Section* previousSection = nullptr;
    Section* currentSection = nullptr;
    
    bool end = false;
    
    while(std::getline(*(this->input),line)) {
        ++lineNumber;

        //TODO: Ako je dosao kraj fajla a nije bilo .end-a mora da se prijavi greska.
        if (line == "EOF") break;

        //If line contains comments we need to remove them
        size_t commentStart = line.find_first_of('#');

        if (commentStart != std::string::npos) {
            line = line.substr(0, commentStart);
        }

        //If line consist only of empty chars, loop is proceeding to the next line.
        if (line.find_last_not_of(Utils::emptyChars) == std::string::npos) {
            continue;
        }
        
        line = Utils::trim(line);
        this->lines[lineNumber] = line;
        
        std::string newLine;
         
        //===================Parsing label=======================
        if (line.find_first_of(':') != std::string::npos) {
            
            //Getting label name
            StringTokenizer st(":");
            st.tokenize(line);     
            if (st.tokenNumber() > 2 || st.tokenNumber() < 1) {
                throw AssemblingException(line.c_str(), lineNumber);
            }

            std::string& token = st.nextToken();
            SymbolTable::const_iterator it = this->symbolTable.find(token);
            
            bool undefined = false;
            if (it != this->symbolTable.end() && currentSection != nullptr) {
                if (it->second->getSection() == SectionType::UDF) {
                    undefined = true;
                }
                else {
                    //Symbol is already defined.
                    throw AssemblingException(line.c_str(), lineNumber);
                }
            }
            if (undefined) {
                it->second->setSection(currentSection->getName());
            }
            else {

                Symbol* s = new Symbol(token, currentSection ? currentSection->getSection() : SectionType::UDF, locationCounter, true);

                this->symbolTable[token] = s;
            }
            if (st.tokenNumber() == 1) {
                continue;
            }
            else {
                newLine = st.nextToken();
            }
        }

        else {
            newLine = line;
        }
        //===================================================================
        
        newLine = Utils::trim(newLine);
        newLine = Utils::removeRepeatingChars(newLine);
        
         //If we read line that equals to .end, we reached end of file.
        if (newLine.compare(".end") == 0) {
            end = true;
            break;
        }
            
        //====================  Parsing section or variable declaration.    ========================
        if (newLine[0] == '.') {
            if (newLine.compare(".global")) {
                if (currentSection != nullptr) {
                    throw AssemblingException("ERROR: Directive .global can only appear outside of section.", newLine, lineNumber);
                } 
                std::string params = this->getParameters(newLine);
                StringTokenizer st = StringTokenizer(",");
                
                st.tokenize(params);
                if (!st.hasNext()) {
                    throw AssemblingException("ERROR: Unallowed use of directive .global.", newLine, lineNumber);
                }
                while (st.hasNext()) {
                    std::string param = st.nextToken();
                    
                    param = Utils::trim(param);
                    if (std::regex_match(param, this->labelRegex)) {
                        if (this->symbolTable.find(param) != this->symbolTable.end()) {
                              throw AssemblingException("ERROR: Symbol is already defined", newLine, lineNumber);
                        }
                        
                        this->symbolTable[param] = new Symbol(param, SectionType::UDF, locationCounter, false);
                    }
                }
            }
            else if (newLine.compare(".text") == 0) {
                if (this->symbolTable.find(newLine) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .text was already defined, at line", line, lineNumber);
                }
                else {
                    this->changeSection(newLine, SectionType::TEXT, Access::EX, locationCounter, previousSection, currentSection);
                }
            }
            
            else if (newLine.compare(".data") == 0) {
                 if (this->symbolTable.find(newLine) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .data was already defined, at line", line, lineNumber);
                }
                else {
                    this->changeSection(newLine, SectionType::DATA, Access::RW, locationCounter, previousSection, currentSection);
                }
            }
            
            else if (newLine.compare(".rodata") == 0) {
                 if (this->symbolTable.find(newLine) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .rodata was already defined, at line", line, lineNumber);
                }
                else {
                    this->changeSection(newLine, SectionType::RO_DATA, Access::RD, locationCounter, previousSection, currentSection);
                }
            }
            
            else if (newLine.compare(".bss") == 0) {
                 if (this->symbolTable.find(newLine) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .bss was already defined, at line", line, lineNumber);
                }
                else {
                    this->changeSection(newLine, SectionType::BSS, Access::RW, locationCounter, previousSection, currentSection);
                }
            }
            
            else {
                parseDirective(newLine, lineNumber, locationCounter);
            }
          
            
            continue;
        }
        //=====================================================================================
        
        //=========================== Parsing instruction =====================================
        Instruction* i = new Instruction();
        i->parseInstruction(newLine, lineNumber);

        this->instructions[lineNumber] = i;
       
    }
}

void Assembler::secondPass() {

}

void Assembler::parseDirective(const std::string& line, const int lineNumber, int& locationCounter, Section* currentSection) {
    
    size_t spacePos = line.find_first_of(" ");

    std::string directive;
    std::string ops;

    if (spacePos != std::string::npos) {
        directive = line.substr(0, spacePos);
        ops = line.substr(spacePos + 1);
    }
    if (directive.compare(".byte") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSection() != SectionType::DATA || currentSection->getSection() != SectionType::RO_DATA)) {
            throw AssemblingException("Directive .byte not allowed in this section.", line, lineNumber);
        }
        locationCounter += 1;
    }
    else if (directive.compare(".word") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSection() != SectionType::DATA || currentSection->getSection() != SectionType::RO_DATA)) {
            throw AssemblingException("Directive .word not allowed in this section.", line, lineNumber);
        }
        locationCounter += 2;
    }
    else if (directive.compare(".long") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSection() != SectionType::DATA || currentSection->getSection() != SectionType::RO_DATA)) {
            throw AssemblingException("Directive .long not allowed in this section.", line, lineNumber);
        }
        locationCounter += 4;
    }

    else if (directive.compare(".skip") == 0) {
        try {
            if (currentSection == nullptr ? true : (currentSection->getSection() != SectionType::DATA || currentSection->getSection() != SectionType::BSS)) {
                throw AssemblingException("Directive .skip not allowed in this section.", line, lineNumber);
            }
            
            int skip = std::stoi(ops);
            
            locationCounter += skip;
        }
        catch (std::exception& e) {
            throw AssemblingException("Invalid use of directive .skip", line, lineNumber);
        }
    }
    
    else if (directive.compare(".align") == 0) {
        //TODO: implement align.
    }

}

void Assembler::changeSection(const std::string& sectionName, SectionType sectionType, Access access, int locationCounter, Section*& previousSection, Section*& currentSection) {

    size_t sectionSize = previousSection ? locationCounter - previousSection->getSectionSize() : locationCounter;

    previousSection = currentSection;
    Symbol* s = new Section(sectionSize, Access::EX, sectionName, sectionType, locationCounter, true);
    currentSection = (Section*)s;

    
    this->symbolTable[sectionName] = s;
}

Assembler::~Assembler() {

    //Closing streams.
    this->input->close();
    this->output->close();

    //Deleting input stream.
    delete this->input;
    this->input = nullptr;

    //Deleting output stream.
    delete this->output;
    this->output = nullptr;
    
    for(auto it = this->symbolTable.begin(); it != this->symbolTable.end(); it++) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        } 
    }
        
    for(auto it = this->instructions.begin(); it != this->instructions.end(); it++) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        } 
    }
       
    
}