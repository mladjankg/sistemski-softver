#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <stdexcept>

#include "assembler.h"
#include "utils.h"
#include "string_tokenizer.h"
#include "symbol.h"
#include "ss_exceptions.h"
#include "instruction.h"
#include "operand.h"
#include "section.h"
#include "directive.h"
#include "asm_declarations.h"
#include "relocation.h"
#include "elf.h"

using namespace ss;

void Assembler::firstPass() {
    std::string line;
    
    
    int locationCounter = 0;
    int lineNumber = 0;
    char sectionCounter = 0;
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

            //Only one label can be defined in one line.
            if (st.tokenNumber() > 3 || st.tokenNumber() < 1) {
                throw AssemblingException(line.c_str(), lineNumber);
            }

            std::string& token = st.nextToken();
            SymbolTable::const_iterator it = this->symbolTable.find(token);
            
            if (Assembler::checkReserved(token)) {
                throw AssemblingException("Label name error, word " + token + " is reserved", line, lineNumber);
            }

            bool undefined = false;

            //Checking if label was already defined. If it was defined in .global directive, we are only
            //changing it's entry in symbol table. If it was defined in some section, error is thrown.
            if (it != this->symbolTable.end() && currentSection != nullptr) {
                if (it->second->getSectionCode() == SectionType::UDF) {
                    undefined = true;
                }
                else {

                    //Symbol is already defined.
                    throw AssemblingException(line.c_str(), lineNumber);
                }
            }
            if (undefined) {
                it->second->setSectionCode(currentSection->getSectionCode());
                it->second->setSectionPtr(currentSection);
                it->second->setOffset(locationCounter);
            }

            else {

                Symbol* s = new Symbol(token, currentSection ? currentSection->getSectionCode() : SectionType::UDF, locationCounter, true);
                s->setSectionPtr(currentSection);
                this->symbolTable[token] = s;
            }
            
            if (st.hasNext()) {
                newLine = st.nextToken();
                if (newLine.compare("") == 0) continue;
                else {
                    if (st.hasNext()) {
                        std::string junk = st.nextToken();
                        if (junk.compare("") != 0) {
                            throw AssemblingException("Syntax error", line, lineNumber);
                        }
                    }
                }
            }
            else {
                continue;
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
            std::string directive = this->getDirective(newLine);
            
            //Line contains .global directive
            if (directive.compare(".global") == 0) {
                if (currentSection != nullptr) {
                    throw AssemblingException("Directive .global can only appear outside of section.", newLine, lineNumber);
                } 

                //Getting labels defined as global.
                std::string params = this->getParameters(newLine);
                StringTokenizer st = StringTokenizer(",");
                
                st.tokenize(params);
                //.global directive must have at least one parameter
                if (!st.hasNext()) {
                    throw AssemblingException("Unallowed use of directive .global (no parameters)", newLine, lineNumber);
                }
                while (st.hasNext()) {
                    std::string param = st.nextToken();
                    
                    param = Utils::trim(param);

                    //Checking if label name is valid
                    if (std::regex_match(param, Utils::labelRegex)) {
                        //Checking if label was already defined as global.
                        if (this->symbolTable.find(param) != this->symbolTable.end()) {
                              throw AssemblingException("ERROR: Symbol is already defined", newLine, lineNumber);
                        }
                        
                        //If all previous checks were succesfull, label can be added to symbol table.
                        this->symbolTable[param] = new Symbol(param, SectionType::UDF, locationCounter, false);
                    }
                }
            }
            //Start of text section
            else if (directive.compare(".text") == 0) {
                if (this->symbolTable.find(directive) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .text was already defined, at line", line, lineNumber);
                }
                else {
                    this->changeSection(directive, SectionType::TEXT, Access::EX, locationCounter, previousSection, currentSection);
                }
            }
            
            //Start of data section
            else if (directive.compare(".data") == 0) {
                 if (this->symbolTable.find(directive) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .data was already defined, at line", line, lineNumber);
                }
                else {
                    this->changeSection(directive, SectionType::DATA, Access::RW, locationCounter, previousSection, currentSection);
                }
            }
            
            //Start of rodata section
            else if (directive.compare(".rodata") == 0) {
                 if (this->symbolTable.find(directive) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .rodata was already defined", line, lineNumber);
                }
                else {
                    this->changeSection(directive, SectionType::RO_DATA, Access::RD, locationCounter, previousSection, currentSection);
                }
            }
            
            //Start of bss section
            else if (directive.compare(".bss") == 0) {
                 if (this->symbolTable.find(directive) != this->symbolTable.end()) {
                    throw AssemblingException("ERROR: Section .bss was already defined", line, lineNumber);
                }
                else {
                    this->changeSection(directive, SectionType::BSS, Access::RW, locationCounter, previousSection, currentSection);
                }
            }
            
            //Line contains directive like .byte .word .long .skip or .align
            else {
                this->parseDirective(newLine, directive, lineNumber, locationCounter, currentSection);
            }
          
            
            continue;
        }
        //=====================================================================================
        
        //=========================== Parsing instruction =====================================
        
        if (currentSection) {

            if (currentSection->getSectionCode() == SectionType::TEXT) {

                Instruction *i = new Instruction();
                i->parseInstruction(newLine, lineNumber);

                this->instructions[lineNumber] = i;

                currentSection->increaseParsed();
                locationCounter += i->getInstructionSize();
                
            }
            else {
                throw AssemblingException("Assembling error", line, lineNumber);
            }
        }
        else {
            throw AssemblingException("Assembling error", line, lineNumber);
        }
    }
    
    if (currentSection != nullptr) {
        currentSection->setSectionSize(previousSection == nullptr ? locationCounter : locationCounter - previousSection->getSectionSize());
    }

    else {
        throw AssemblingException("No section defined in file.");
    }

    if (!end) {
        throw AssemblingException("End of file reached, but .end directive was not found.");
    }
}

void Assembler::parseDirective(const std::string& line, const std::string& directive, const int lineNumber, int& locationCounter, Section* currentSection) {
    
    size_t spacePos = line.find_first_of(" ");

    Directive *d = nullptr;
    
    std::string ops;

    if (spacePos != std::string::npos) {
        ops = line.substr(spacePos + 1);
    }
    bool hasOps = ops.find_first_not_of(Utils::emptyChars) != std::string::npos;

    bool skip = false;
    bool align = false;
    bool bwl = false;
    if (directive.compare(".byte") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSectionCode() == SectionType::TEXT)) {
            throw AssemblingException("Directive .byte not allowed in this section", line, lineNumber);
        }
        
        //locationCounter += 1;

        d = new BWLDirective(DirectiveType::BYTE);
        bwl = true;
    }
    else if (directive.compare(".word") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSectionCode() == SectionType::TEXT)) {
            throw AssemblingException("Directive .word not allowed in this section", line, lineNumber);
        }
       
        //locationCounter += 2;
       
        if (currentSection->getSectionCode() != SectionType::BSS)
        d = new BWLDirective(DirectiveType::WORD);
        bwl = true;
    }
    else if (directive.compare(".long") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSectionCode() == SectionType::TEXT)) {
            throw AssemblingException("Directive .long not allowed in this section", line, lineNumber);
        }

        //locationCounter += 4;
    
        d = new BWLDirective(DirectiveType::LONG);
        bwl = true;
    }
    else if (directive.compare(".skip") == 0) {

        if (currentSection == nullptr ? true : (currentSection->getSectionCode() != SectionType::DATA && currentSection->getSectionCode() != SectionType::BSS)) {
            throw AssemblingException("Directive .skip not allowed in this section", line, lineNumber);
        }
        
        if (!std::regex_match(ops, Operand::decimalRegex)) {
            throw AssemblingException("Invalid use of directive .skip", line, lineNumber);
        }
        
        unsigned int skip = std::stoi(ops);

        locationCounter += skip;

        if (currentSection->getSectionCode() == SectionType::DATA) {
            d = new SkipDirective();
            ((SkipDirective*)d)->setOffset(skip);
        }
    }
    
    else if (directive.compare(".align") == 0) {
        if (currentSection->getNParsed() != 0) {
            throw AssemblingException("Directive .align can only be written on the beggining of the section.", line, lineNumber);
        }

        if (!std::regex_match(ops, Utils::decimalRegex)) {
            throw AssemblingException(".align invalid operand", line, lineNumber);
        }

        int pow = std::stoi(ops);
        
        if (pow > 14) {
            throw AssemblingException(".align operand out of range", line, lineNumber);
        }
        unsigned int align = 1;
        align <<= pow;
        currentSection->setAlign(align);
    }

    currentSection->increaseParsed();

    if (currentSection->getSectionCode() == SectionType::BSS) {
        if (hasOps && (directive.compare(".skip") != 0)) {
            throw AssemblingException("Cannot initialize data in .bss section", line, lineNumber);
        }
        else {
            return;
        }
    }

    if (bwl) {
        StringTokenizer st(",");
        st.tokenize(ops);
        BWLDirective* bwld = (BWLDirective*)d;
        while(st.hasNext()) {
            std::string op = Utils::trim(st.nextToken());
            if (!std::regex_match(op, Utils::labelRegex) && !std::regex_match(op, Utils::decimalRegex)) {
                throw AssemblingException("Invalid operand", line, lineNumber);
            }

            bwld->setOperand(op);
        }
        char multiplicator = bwld->getType() == DirectiveType::BYTE ? 1 : bwld->getType() == DirectiveType::WORD ? 2 : 4;
        locationCounter += bwld->getOperands().size() * multiplicator;
    }

    if (d != nullptr) {
        if (currentSection->getSectionCode() == SectionType::DATA) {
            this->data[lineNumber] = d;
        }
        if (currentSection->getSectionCode() == SectionType::RO_DATA) {
            this->roData[lineNumber] = d;
        }
    }
}

void Assembler::changeSection(const std::string& sectionName, SectionType sectionType, Access access, int locationCounter, Section*& previousSection, Section*& currentSection) {
    
    size_t sectionSize = previousSection ? locationCounter - previousSection->getSectionSize() : locationCounter;

    previousSection = currentSection;
    if (previousSection != nullptr) {
        previousSection->setSectionSize(sectionSize);
    }

    Symbol* s = new Section(0, access, sectionName, sectionType, locationCounter, true);
    
    currentSection = (Section*)s;

    
    this->symbolTable[sectionName] = s;
    this->sectionOrder[sectionCounter++] = sectionType;
}

std::string Assembler::getParameters(const std::string line) {
    
    std::string formatted(line); 
    formatted = Utils::trim(formatted);
    
    size_t spacePos = formatted.find_first_of(' ');
    if (spacePos == std::string::npos) {
        return "";
    }
    
    formatted = formatted.substr(spacePos + 1);
    return Utils::trim(formatted);
}

std::string Assembler::getDirective(const std::string line) const {
    if (line[0] != '.') {
        return Utils::empty;
    }
    
    
    if (line.find_first_not_of(Utils::emptyChars) == std::string::npos) {
        return Utils::empty;    
    }
    
    
    std::string newLine = Utils::trim(line);
    size_t spacePos = newLine.find_first_of(' ');
    
    return newLine.substr(0, spacePos);
 
}