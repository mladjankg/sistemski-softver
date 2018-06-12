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
            if ((op1->getAddressing() == AddressingCode::IMMED) && (instr->getInstruciton() != InstructionCode::CALL) && (op1->getType() != OperandType::PSW)) {
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

        int byte = (firstHalf >> 8 & 0xFF);
        if (!(byte & 0xF0)) relStream << '0';
        relStream << std::hex << byte << " ";
        this->textBin.push_back((char)byte);

        byte = firstHalf & 0xFF;
        if (!(byte & 0xF0)) relStream << '0';
        relStream << std::hex << byte << " ";
        this->textBin.push_back((char)byte);

        if (instr->getInstructionSize() == 4) {
            byte = (secondHalf >> 8) & 0xFF;
            if (!(byte & 0xF0)) relStream << '0';
            relStream << std::hex << byte << " ";
            this->textBin.push_back((char)byte);
            
            byte = secondHalf & 0xFF;
            if (!(byte & 0xF0)) relStream << '0';
            relStream << std::hex << byte << " ";
            this->textBin.push_back((char)byte);
        }

    }

    this->textOut = relStream.str();
}

void Assembler::assembleDataSection(Section* current, size_t& locationCounter) {
    std::stringstream relStream;

    std::map<int, Directive*> *section;
    std::vector<char> *binData;
    if (current->getSectionCode() == SectionType::RO_DATA) {
        relStream << "#rodata\n";
        section = &this->roData;
        binData = &this->roDataBin;
    }
    else if (current->getSectionCode() == SectionType::DATA) {
        relStream << "#data\n";
        section = &this->data;
        binData = &this->dataBin;
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
            char t = 0;
            for (int i = 0; i < size; ++i) {
                relStream << "00 ";
                binData->push_back(t);
            }
            locationCounter += size;
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
                                binData->push_back((char)val);
                            }
                        }

                        else if (bwl->getType() == DirectiveType::WORD) {
                            if (val & LIMIT_MASK) valid = false;
                            else {
                                relStream << std::setfill('0') << std::setw(2) << std::hex << (val & 0xFF) << ' ';
                                binData->push_back((char)(val & 0xFF));
                                
                                relStream << std::setfill('0') << std::setw(2) << std::hex << (val >> 8) << ' ';
                                binData->push_back((char)(val >> 8));
                            }
                        }

                        else if (bwl->getType() == DirectiveType::LONG) {
                            relStream << std::setfill('0') << std::setw(2) << std::hex << (val & 0xFF) << ' ';
                            binData->push_back((char)(val & 0xFF));
                            
                            relStream << std::setfill('0') << std::setw(2) << std::hex << ((val >> 8) & 0xFF) << ' ';
                            binData->push_back((char)((val >> 8) & 0xFF));

                            relStream << std::setfill('0') << std::setw(2) << std::hex << ((val >> 16) & 0xFF) << ' ';
                            binData->push_back((char)((val >> 16) & 0xFF));
                            
                            relStream << std::setfill('0') << std::setw(2) << std::hex << ((val >> 24) & 0xFF) << ' ';
                            binData->push_back((char)((val >> 24) & 0xFF));
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
                        binData->push_back((char)(offset & 0xFF));

                        relStream << std::setfill('0') << std::setw(2) << std::hex << (offset >> 8) << ' ';
                        binData->push_back((char)(offset & 0xFF));
                    }

                    else if (bwl->getType() == DirectiveType::LONG) {
                        relStream << std::setfill('0') << std::setw(2) << std::hex << (offset & 0xFF) << ' ';
                        binData->push_back((char)(offset & 0xFF));
                        
                        relStream << std::setfill('0') << std::setw(2) << std::hex << ((offset >> 8) & 0xFF) << ' ';
                        binData->push_back((char)((offset >> 8) & 0xFF));

                        relStream << "00 ";
                        relStream << "00 ";
                        binData->push_back(0);
                        binData->push_back(0);

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
            if (op->getType() == OperandType::PSW) {
                firstHalf = 0x7;
            }
            else if (op->getType() == OperandType::IMMED_VAL) {

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
                short offset = this->resolveLabel(locationCounter, current, op1Raw, lineNumber, (instr->getInstruciton() == InstructionCode::ADD_JMP) || (instr->getInstruciton() == InstructionCode::CALL)) ;

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

                    short offset = this->resolveLabel(locationCounter, current, off, lineNumber);

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

short Assembler::resolveLabel(const size_t& locationCounter, Section* current, const std::string lab, const int lineNumber, const bool pcRel) {
    
    std::string label(lab);
 
    if (lab[0] == '&' || lab[0] == '$') {
        label = label.substr(1);
    }
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

            Relocation rel(relOffset, RelocationType::R_386_PC16, s->getNo());
            this->relText.push_back(rel);

            this->txtRelText.push_back(relocationRecord);
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

        Relocation rel(relOffset, RelocationType::R_386_16, s->getNo());
        this->relText.push_back(rel);

        this->txtRelText.push_back(relocationRecord);
    }

    return offset;
}

short Assembler::resolveDataLabel(const size_t& locationCounter, Section* current, const std::string lab, BWLDirective* bwl, const int lineNumber) {
    std::string label(lab);
 
    if (lab[0] == '&' || lab[0] == '$') {
        label = label.substr(1);
    }
    
    Symbol* s = this->symbolTable[label];

    if (s == nullptr) {
        throw AssemblingException("Unknown symbol", this->lines[lineNumber], lineNumber);
    }
    short offset = 0;
    size_t relOffset = locationCounter - current->getOffset() - (bwl->getType() == DirectiveType::WORD ? 2 : 4);
    if (s->isLocal()) {
        offset = relOffset;
        
        if (s->getSectionPtr() != nullptr)
            s = s->getSectionPtr();

        if (s == nullptr) {
            throw AssemblingException("Unknown error, method resolveDataLabel");
        }
    }

    std::string relTypeStr;
    RelocationType relType;

    if (bwl->getType() == DirectiveType::WORD) {
        relTypeStr = "R_386_16";
        relType = RelocationType::R_386_16;
    }
    else if (bwl->getType() == DirectiveType::LONG) {
        relTypeStr = "R_386_32";
        relType = RelocationType::R_386_32;
    } 
    else {
        throw AssemblingException("Cannot rellocate this directive type", this->lines[lineNumber], lineNumber);
    }

    std::stringstream relStream;

    relStream << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << std::hex << relOffset 
              << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << relTypeStr
              << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << s->getNo();

    Relocation rel(relOffset, relType, s->getNo());

    std::string relocationRecord(relStream.str());

    if (current->getSectionCode() == SectionType::DATA) {
        this->txtRelData.push_back(relocationRecord);
        this->relData.push_back(rel);
    }
    else if (current->getSectionCode() == SectionType::RO_DATA) { 
        this->txtRelROData.push_back(relocationRecord);
        this->relROData.push_back(rel);
    }
    else {
        throw AssemblingException("Unsupported section in method resolveDataLabel", this->lines[lineNumber], lineNumber);
    }

    return offset;
}