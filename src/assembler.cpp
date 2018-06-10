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

void Assembler::writeOutput() {
    bool hasText = false;
    bool hasRoData = false;
    bool hasData = false;

    for(int i = 0; i < SECTION_NUMBER && this->sectionOrder[i] != SectionType::UDF; ++i) {
        if (this->sectionOrder[i] == SectionType::TEXT) 
            hasText = true;

        if (this->sectionOrder[i] == SectionType::RO_DATA)
            hasRoData = true;
        
        if (this->sectionOrder[i] == SectionType::DATA)
            hasData = true;
    }

    if (hasText) {
        *this->output << this->textOut << std::endl;
    }
    if (hasRoData) {
        *this->output << this->roDataOut << std::endl;
    }
    if (hasData) {
        *this->output << this->dataOut << std::endl;
    }

    if (this->symbolTable.size() != 0) {
        *this->output << "#symbol table" << std::endl << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "#name" << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) 
                                                      << "section" << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) 
                                                      << "value" << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH)  
                                                      << "no" << std::endl;

        for(auto it = this->symbolTable.begin(); it != this->symbolTable.end(); ++it) {
            Symbol *s = it->second;
            
            std::string symbStr = s->toString();
            *this->output << symbStr << std::endl;
        }
    }
    
    if (hasText && this->relText.size() != 0) {
        *this->output << "#.ret.text" << std::endl 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "offset" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "tip" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "vr" << std::endl; 

        for(auto line:this->relText) {
            *this->output << line << std::endl;
        }
    }

    if (hasRoData && this->relROData.size() != 0) {
        *this->output << "#.ret.rodata" << std::endl 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "offset" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "tip" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "vr" << std::endl; 
        for(auto line:this->relROData) {
            *this->output << line << std::endl;
        }
    }

    if (hasData && this->relData.size() != 0) {
        *this->output << "#.ret.data" << std::endl 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "offset" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "tip" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "vr" << std::endl;  
        for(auto line:this->relData) {
            *this->output << line << std::endl;
        }
    }

}

void Assembler::assemble() {
    this->firstPass();
    this->secondPass();
    this->writeOutput();
}
    
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
                std::cout << this->textOut;
                break;
            }
            case SectionType::DATA: {
                this->assembleDataSection(currentSection, locationCounter);
                break;
            }
            case SectionType::RO_DATA: {
                this->assembleDataSection(currentSection, locationCounter);
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
    
    std::stringstream relStream;
             
    relStream << "#text\n";
    
    for (auto it = this->instructions.begin(); it != this->instructions.end(); ++it) {
        Instruction* instr = it->second;
        
        //First two and potential second two bytes of instruction
        short firstHalf = 0, secondHalf = 0;
        locationCounter += instr->getInstructionSize();

        //Writting condition code to the first part of instruction
        const short condCode =(short) instr->getCondition();
        firstHalf |= condCode << CONDITION_FLAGS_OFFSET;

        //Writting instruction code
        const short instrCode = (instr->getInstruciton() != InstructionCode::ADD_JMP ? instr->getInstruciton() : InstructionCode::ADD);
        firstHalf |= instrCode << INSTRUCTION_FLAGS_OFFSET;


        if (Instruction::operandNumber[instr->getInstruciton()] != 0) {      

            //Getting first operand            
            Operand* op1 = instr->getOperand1();
            auto op1Addr = op1->getAddressing();

            //Only call is allowed to have first operand provided with immediate addressing.
            if ((op1->getAddressing() == AddressingCode::IMMED) && (instr->getInstruciton() != InstructionCode::CALL)) {
                std::string line = this->lines[it->first];
                throw AssemblingException("Addressing error, immediate operand cannot be destination", line, it->first);
            }

            //Writting addressing flags
            short addrCode = op1Addr;
            firstHalf |= addrCode << OP1_ADDRESSING_FLAGS_OFFSET;

            char op1Flags = this->getOperandCode(op1, current, instr, locationCounter, secondHalf, it->first);

            firstHalf |= op1Flags << 5;

            Operand* op2 = instr->getOperand2();
            
            if (op2 != nullptr) {
                auto op2Addr = op2->getAddressing();

                addrCode = op2Addr;
                firstHalf |= addrCode << OP2_ADDRESSING_FLAGS_OFFSET;

                char op2Flags = this->getOperandCode(op2, current, instr, locationCounter, secondHalf, it->first);
                firstHalf |= op2Flags;
            }
        }

        int byte = firstHalf >> 8 & 0xFF;
        if (!(byte & 0xF0)) relStream << '0';
        relStream << std::hex << byte << " ";

        byte = firstHalf && 0xFF;
        if (!(byte & 0xF0)) relStream << '0';
        relStream << std::hex << byte << " ";

        if (instr->getInstructionSize() == 4) {
            byte = (secondHalf >> 8) & 0xFF;
            if (!(byte & 0xF0)) relStream << '0';
            relStream << std::hex << byte << " ";

            byte = secondHalf & 0xFF;
            if (!(byte & 0xF0)) relStream << '0';
            relStream << std::hex << byte << " ";
        }

        std::cout << std::flush;
    }

    this->textOut = relStream.str();
}

void Assembler::assembleDataSection(Section* current, size_t& locationCounter) {
    std::stringstream relStream;

    std::map<int, Directive*> *section;
    if (current->getSectionCode() == SectionType::RO_DATA) {
        relStream << "#rodata\n";
        section = &this->roData;
    }
    else if (current->getSectionCode() == SectionType::DATA) {
        relStream << "#data\n";
        section = &this->data;
    }
    else {
        throw AssemblingException("Unsupported section in method assembleDataSection");
    }
    
    
    for (auto it = section->begin(); it != section->end(); ++it) {
        Directive* d = it->second;

        if (d->getType() == DirectiveType::SKIP) {
            if (current->getSectionCode() == SectionType::RO_DATA) {
                throw AssemblingException("Directive skip is not supported in rodata seciton", this->lines[it->first], it->first);
            }

            SkipDirective* sd = (SkipDirective*)d;

            unsigned int size = sd->getOffset();
            for (int i = 0; i < size; ++i) {
                relStream << "00 ";
            }
            locationCounter += size;
        }
        else if (d->getType() == DirectiveType::ALIGN) {
            
        }
        else {
            BWLDirective* bwl = (BWLDirective*)d;
            auto operands = bwl->getOperands();

            if (operands.size() == 0 && current->getSectionCode() == SectionType::RO_DATA) {
                throw AssemblingException("Data in rodata section must be initialized", this->lines[it->first], it->first);
            }
            for(auto op: operands) {
                if (std::regex_match(op, Utils::decimalRegex)) {
                    bool valid = true;
                    try {
                        int val = std::stoi(op);

                        locationCounter += bwl->getType() == DirectiveType::BYTE ? 1 : bwl->getType() == DirectiveType::WORD ? 2 : 4;
                        if (bwl->getType() == DirectiveType::BYTE) {
                            if (val & MAX_BYTE_MASK) valid = false; 
                            else {
                                relStream << std::setfill('0') << std::setw(2) << std::hex << val << ' ';
                                
                            }
                        }

                        else if (bwl->getType() == DirectiveType::WORD) {
                            if (val & LIMIT_MASK) valid = false;
                            else {
                                relStream << std::setfill('0') << std::setw(2) << std::hex << (val & 0xFF) << ' ';
                                relStream << std::setfill('0') << std::setw(2) << std::hex << (val >> 8) << ' ';
                                
                            }
                        }

                        else if (bwl->getType() == DirectiveType::LONG) {
                            relStream << std::setfill('0') << std::setw(2) << std::hex << (val & 0xFF) << ' ';
                            relStream << std::setfill('0') << std::setw(2) << std::hex << ((val >> 8) & 0xFF) << ' ';
                            relStream << std::setfill('0') << std::setw(2) << std::hex << ((val >> 16) & 0xFF) << ' ';
                            relStream << std::setfill('0') << std::setw(2) << std::hex << ((val >> 24) & 0xFF) << ' ';
                            
                        }
                    }
                    catch(std::invalid_argument e) {
                        valid = false;
                    }

                    if (!valid) {
                        throw AssemblingException("Argument out of range", this->lines[it->first], it->first);
                    }
                }
                else if (std::regex_match(op, Utils::labelRegex)) {
                    if (bwl->getType() == DirectiveType::BYTE) {
                        throw AssemblingException("Cannot initialize byte with possible word", this->lines[it->first], it->first);
                    }
                    locationCounter += bwl->getType() == DirectiveType::WORD ? 2 : 4;
                    short offset = this->resolveDataLabel(locationCounter, current, op, bwl, it->first);

                    if (bwl->getType() == DirectiveType::WORD) {

                        relStream << std::setfill('0') << std::setw(2) << std::hex << (offset & 0xFF) << ' ';
                        relStream << std::setfill('0') << std::setw(2) << std::hex << (offset >> 8) << ' ';

                    }

                    else if (bwl->getType() == DirectiveType::LONG) {
                        relStream << std::setfill('0') << std::setw(2) << std::hex << (offset & 0xFF) << ' ';
                        relStream << std::setfill('0') << std::setw(2) << std::hex << ((offset >> 8) & 0xFF) << ' ';
                        relStream << "00 ";
                        relStream << "00 ";

                    }
                }
                else {
                    throw AssemblingException("Unknown operand", this->lines[it->first], it->first);
                }
            }
        }
    }
    if (current->getSectionCode() == SectionType::RO_DATA) {
        this->roDataOut = relStream.str();
    }
    else {
        this->dataOut = relStream.str();
    }
}

char Assembler::getOperandCode(Operand* op, Section* current, Instruction* instr,  const size_t& locationCounter, short& secondHalf, const int lineNumber) {

    AddressingCode op1Addr = op->getAddressing();
    const std::string op1Raw = op->getRawText();
    char firstHalf = 0;
    switch(op1Addr) {
        case AddressingCode::REGDIR: {
            char regNum = op1Raw[1] - '0';
            firstHalf = regNum;
            break;
        }
    
        case AddressingCode::IMMED: {
            if (op->getType() == OperandType::IMMED_VAL) {

                short val = 0;
                if (!(this->getImmediateValue(op1Raw, val))) {
                    std::string line = this->lines[lineNumber];
                    throw AssemblingException("Argument out of bounds", line, lineNumber);
                }

                secondHalf = SWAP_BYTES((short)val);
            }
            else if (op->getType() == OperandType::LABEL_VAL) {
                std::string label = op1Raw.substr(1);

                short offset = this->resolveLabel(locationCounter, current, label, lineNumber);

                secondHalf = SWAP_BYTES(offset);
            }
            break;
        }

        case AddressingCode::MEMDIR: {
            if (op->getType() == OperandType::MEMDIR_VAL) {
                short offset = this->resolveLabel(locationCounter, current, op1Raw, lineNumber, instr->getInstruciton() == InstructionCode::ADD_JMP);

                secondHalf = SWAP_BYTES(offset);
            }

            else if (op->getType() == OperandType::DECIMAL_LOCATION_VAL) {
                short val = 0;

                if (!this->getImmediateValue(op1Raw.substr(1), val))
                {
                    std::string line = this->lines[lineNumber];
                    throw AssemblingException("Argument out of bounds", line, lineNumber);
                }

                secondHalf = SWAP_BYTES((short)val);
            }
            break;
        }

        case AddressingCode::REGINDPOM: {
            size_t leftBracket = op1Raw.find_first_of('[');
            std::string reg;
            std::string off;
            char regNum = 0;
            
            if (op->getType() == OperandType::PCREL_VAL) {
                regNum = 7;
                off = op1Raw.substr(1);

                short offset = this->resolveLabel(locationCounter, current, op1Raw, lineNumber, true);
                secondHalf = SWAP_BYTES(offset);
            }

            else {
                reg = op1Raw.substr(0, leftBracket);
                off = op1Raw.substr(leftBracket + 1, op1Raw.find_first_of(']') - leftBracket - 1);

                regNum = reg[1] - '0';
                if (op->getType() == OperandType::REGIND_DEC_VAL) {
                    short val = 0;

                    if (!this->getImmediateValue(off, val)) {
                        std::string line = this->lines[lineNumber];
                        throw AssemblingException("Argument out of bounds", line, lineNumber);
                    }

                    secondHalf = SWAP_BYTES((short)val);
                }

                if (op->getType() == OperandType::REGIND_LAB_VAL) {
                    short offset = this->resolveLabel(locationCounter, current, op1Raw, lineNumber);

                    secondHalf = SWAP_BYTES(offset);
                }
            }

            firstHalf = regNum;
            break;
        }
    }

    return firstHalf;
}

bool Assembler::getImmediateValue(const std::string strVal, short& immed) {
    int val = 0;

    try {
        val = std::stoi(strVal);
    }
    catch (std::invalid_argument e) {
        return false;
    }

    if (val & LIMIT_MASK) {
        return false;
    }

    immed = (short)val;
    return true;
}

short Assembler::resolveLabel(const size_t& locationCounter, Section* current, const std::string label, const int lineNumber, const bool pcRel) {
    
    if (this->symbolTable.find(label) == this->symbolTable.end()) {
        std::string line = this->lines[lineNumber];
        throw AssemblingException("Undefined label", line, lineNumber);
    }

    Symbol* s = this->symbolTable[label];

    short offset = 0;

    if (pcRel) {
        if (current->getSectionCode() != SectionType::TEXT) {
            std::string line = this->lines[lineNumber];
            throw AssemblingException("Cannot have PC relative addressing outside text section", line, lineNumber);
        }

        if (s->getSectionCode() == current->getSectionCode()) {
            offset = s->getOffset() - locationCounter;
        }

        else {
            short relOffset = locationCounter - 2;

            if (s->isLocal()) {
                offset = s->getOffset() - s->getSectionPtr()->getOffset() - 2;
                s = s->getSectionPtr();
            }
            
            else {
                offset = -2;
            }

            std::stringstream relStream;
             
            relStream << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << std::hex << relOffset 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "R_386_PC16" 
                      << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << s->getNo();
            std::string relocationRecord(relStream.str());

            this->relText.push_back(relocationRecord);
        }
    }
    else {
        short relOffset = locationCounter - 2;

        if (s->isLocal()) {
            offset = s->getOffset() - s->getSectionPtr()->getOffset();
            s = s->getSectionPtr();
        }
        else {
            offset = 0;
        }

        std::stringstream relStream;
             
        relStream << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << std::hex << relOffset 
                  << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << "R_386_16" 
                  << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << s->getNo();
        std::string relocationRecord(relStream.str());

        this->relText.push_back(relocationRecord);
    }

    return offset;
}

short Assembler::resolveDataLabel(const size_t& locationCounter, Section* current, const std::string label, BWLDirective* bwl, const int lineNumber) {
    Symbol* s = this->symbolTable[label];

    if (s == nullptr) {
        throw AssemblingException("Unknown symbol", this->lines[lineNumber], lineNumber);
    }
    short offset = 0;
    size_t relOffset = locationCounter - current->getOffset() - (bwl->getType() == DirectiveType::WORD ? 2 : 4);
    if (s->isLocal()) {
        offset = relOffset;
        s = s->getSectionPtr();
    }

    std::string relType;
    if (bwl->getType() == DirectiveType::WORD) {
        relType = "R_386_16";
    }
    else if (bwl->getType() == DirectiveType::LONG) {
        relType = "R_386_32";
    } 
    else {
        throw AssemblingException("Cannot rellocate this directive type", this->lines[lineNumber], lineNumber);
    }

    std::stringstream relStream;

    relStream << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << std::hex << relOffset 
              << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << relType 
              << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << s->getNo();
    std::string relocationRecord(relStream.str());

    if (current->getSectionCode() == SectionType::DATA) {
        this->relData.push_back(relocationRecord);
    }
    else if (current->getSectionCode() == SectionType::RO_DATA) { 
        this->relROData.push_back(relocationRecord);
    }
    else {
        throw AssemblingException("Unsupported section in method resolveDataLabel", this->lines[lineNumber], lineNumber);
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