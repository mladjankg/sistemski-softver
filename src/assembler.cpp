#include <string>
#include <fstream>
#include <map>
#include <list>
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
#include "directive.h"

using namespace ss;

Assembler::Assembler(std::ifstream* in, std::ofstream* out) : input(in), output(out) {

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
            //Only one label can be defined in one line.
            if (st.tokenNumber() > 2 || st.tokenNumber() < 1) {
                throw AssemblingException(line.c_str(), lineNumber);
            }

            std::string& token = st.nextToken();
            SymbolTable::const_iterator it = this->symbolTable.find(token);
            
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
            }

            else {

                Symbol* s = new Symbol(token, currentSection ? currentSection->getSectionCode() : SectionType::UDF, locationCounter, true);
                s->setSectionPtr(currentSection);
                this->symbolTable[token] = s;
            }
            
            if (st.hasNext()) {
                newLine = st.nextToken();
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
                parseDirective(newLine, directive, lineNumber, locationCounter, currentSection);
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
        
        locationCounter += 1;

        d = new BWLDirective(DirectiveType::BYTE);
        bwl = true;
    }
    else if (directive.compare(".word") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSectionCode() == SectionType::TEXT)) {
            throw AssemblingException("Directive .word not allowed in this section", line, lineNumber);
        }
       
        locationCounter += 2;
        //TODO: SREDI SUTRA OVO OVDE DA RADI U BSSU ITD ITD
        if (currentSection->getSectionCode() != SectionType::BSS)
        d = new BWLDirective(DirectiveType::WORD);
        bwl = true;
    }
    else if (directive.compare(".long") == 0) {
        if (currentSection == nullptr ? true : (currentSection->getSectionCode() == SectionType::TEXT)) {
            throw AssemblingException("Directive .long not allowed in this section", line, lineNumber);
        }

        locationCounter += 4;
    
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
        //TODO: implement align.
    }

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
        while(st.hasNext()) {
            ((BWLDirective*)d)->setOperand(Utils::trim(st.nextToken()));
        }
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
    previousSection->setSectionSize(sectionSize);
    
    Symbol* s = new Section(0, access, sectionName, sectionType, locationCounter, true);
    
    currentSection = (Section*)s;

    
    this->symbolTable[sectionName] = s;
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



void Assembler::secondPass() {
    Section* currentSection = nullptr;
    size_t locationCounter = 0;
    unsigned int lineNumber = 0;

    for (char i = 0; i < SECTION_NUMBER || this->sectionOrder[i] == SectionType::UDF; ++i) {
        std::string sectionName = this->sectionOrder[i] == SectionType::TEXT ? ".text" :
            this->sectionOrder[i] == SectionType::DATA ? ".data" :
            this->sectionOrder[i] == SectionType::RO_DATA ? ".rodata" : ".bss";
        
        currentSection = (Section*)this->symbolTable[sectionName];

        switch(this->sectionOrder[i]) {
            case SectionType::TEXT: {
                this->assembleTextSection(currentSection, locationCounter);
                break;
            }
            case SectionType::DATA: {
                break;
            }
            case SectionType::RO_DATA: {
                break;
            }
            case SectionType::BSS: {
                locationCounter += currentSection->getSectionSize();
                break;
            }
        }
    }

}

void Assembler::assembleTextSection(Section* current, size_t& locationCounter) {
    for (auto it = this->instructions.begin(); it != this->instructions.end(); ++it) {
        Instruction* instr = it->second;
        
        //First two and potential second two bytes of instruction
        short firstHalf = 0, secondHalf = 0;
        locationCounter += instr->getInstructionSize();

        //Writting condition code to the first part of instruction
        short condCode =(short) instr->getCondition();
        firstHalf |= condCode << CONDITION_FLAGS_OFFSET;

        //Writting instruction code
        short instrCode = (short) instr->getInstruciton();
        firstHalf |= instrCode << INSTRUCTION_FLAGS_OFFSET;


        if (Instruction::operandNumber[instr->getInstruciton()] != 0) {      

            //Getting first operand            
            Operand* op1 = instr->getOperand1();
            AddressingCode op1Addr = op1->getAddressing();

            //Only call is allowed to have first operand provided with immediate addressing.
            if ((op1->getAddressing() == op1Addr) && (instr->getInstruciton() == InstructionCode::CALL)) {
                std::string line = this->lines[it->first];
                throw AssemblingException("Addressing error, immediate operand cannot be destination", line, it->first);
            }

            //Writting addressing flags
            short addrCode = (short) op1Addr;
            firstHalf |= instrCode << ADDRESSING_FLAGS_OFFSET;

            const std::string op1Raw = op1->getRawText();

            if (op1Addr == AddressingCode::REGDIR) {
                short regNum = op1Raw[1] - 0;
                firstHalf |= regNum << 5;
            }

            else if (op1Addr == AddressingCode::IMMED) {

            }

            else if (op1Addr == AddressingCode::MEMDIR) {

            }
            else { //REGINDPOM

            }
        } 
    }
}

size_t Assembler::resolveLabel(const size_t& locationCounter, Section* current, const std::string label) {
    Symbol* s = this->symbolTable[label];

    size_t offset = 0;
    if (s->getSectionCode() == current->getSectionCode()) {
        offset = s->getOffset() - locationCounter;
    }
    else {
        if (s->getSectionCode() == SectionType::UDF) {
            offset = -2;
        }
        else {
            size_t sectionOffset = s->getOffset() - s->getSectionPtr()->getOffset();
            offset = sectionOffset - 2;

        }
    }

    return offset;
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