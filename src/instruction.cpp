#include <regex>
#include "utils.h"
#include "instruction.h"
#include "string_tokenizer.h"
#include "operand.h"

using namespace ss;

std::regex Instruction::al("(([A-Za-z]{2})|([A-Za-z]{3})|([A-Za-z]{4}))(al$)");
std::regex Instruction::eq("(([A-Za-z]{2})|([A-Za-z]{3})|([A-Za-z]{4}))(eq$)");
std::regex Instruction::ne("(([A-Za-z]{2})|([A-Za-z]{3})|([A-Za-z]{4}))(ne$)");
std::regex Instruction::gt("(([A-Za-z]{2})|([A-Za-z]{3})|([A-Za-z]{4}))(gt$)");
std::regex Instruction::unc("(^(([A-Za-z]{2})|([A-Za-z]{3})|([A-Za-z]{4}))$)");

void Instruction::parseInstruction(std::string line) throw() {
    
    size_t spPos = line.find_first_of(' ');

    if (spPos == std::string::npos) {
        //ret, iret
        size = 2;

    }


    std::string mnemonic = line.substr(0, spPos);
    std::string operands = line.substr(spPos + 1);

    //Parsing instruction condition.    
    bool sufix = false;
    if (std::regex_match(line, al)) {
        sufix = true;
        this->condition = ConditionCode::AL;
    }

    else if (std::regex_match(line, eq)) {
        sufix = true;
        this->condition = ConditionCode::EQ;
    }

    else if (std::regex_match(line, ne)) {
        sufix = true;
        this->condition = ConditionCode::NE;
    }

    else if (std::regex_match(line, gt)) {
        sufix = true;
        this->condition = ConditionCode::GT;
    }

    else if (std::regex_match(line, unc)) {
        this->condition = ConditionCode::AL;
    } 

    else {
        throw AssemblingException("Unknown instruction", line);
    }   
    
    if (sufix) {
        mnemonic = mnemonic.substr(0, line.length() - 2);
    }

    //Parsing instruction mnemonic
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(), ::tolower);

    bool instructionValid = false;

    switch(mnemonic.length()) {
        case 2: {
            if (mnemonic.compare("or")) {
                this->instruction = InstructionCode::OR;
                instructionValid = true;
            }
            break;
        }
        case 3: {
            if (mnemonic.compare("mov")) {
                this->instruction = InstructionCode::MOV;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("add")) {
                this->instruction = InstructionCode::ADD;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("sub")) {
                this->instruction = InstructionCode::SUB;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("pop")) {
                this->instruction = InstructionCode::CMP;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("cmp")) {
                this->instruction = InstructionCode::CMP;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("mul")) {
                this->instruction = InstructionCode::MUL;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("div")) {
                this->instruction = InstructionCode::DIV;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("and")) {
                this->instruction = InstructionCode::AND;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("not")) {
                this->instruction = InstructionCode::NOT;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("shl")) {
                this->instruction = InstructionCode::SHL;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("ret")) {
                this->instruction = InstructionCode::SHR;
                instructionValid = true;
            }

            else if (mnemonic.compare("ret")) {
                this->instruction = InstructionCode::RET;
                instructionValid = true;
            }

            break;
        }
        case 4: {
            if (mnemonic.compare("push")) {
                this->instruction = InstructionCode::PUSH;
                instructionValid = true;
            }
            else if (mnemonic.compare("test")) {
                this->instruction = InstructionCode::TEST;
                instructionValid = true;
            }
            else if (mnemonic.compare("call")) {
                this->instruction = InstructionCode::CALL;
                instructionValid = true;
            }
            else if (mnemonic.compare("iret")) {
                this->instruction = InstructionCode::IRET;
                instructionValid = true;
            }
            break;
        }
    }

    if (!instructionValid) {
        throw AssemblingException("Unknown instruction", line);
    }
    
    //Parsing operands  
    size_t commaPos;
    if ((commaPos = operands.find_first_of(',')) != std::string::npos) {
        StringTokenizer st(",");
        st.tokenize(operands);

        if (st.tokenNumber() > 2) {
            throw AssemblingException("Unknown instruction", line);
        }
        
    

    }
    else {
        operands = Utils::trim(operands);
        if (operands.find_first_of(Utils::emptyChars)) {
            throw AssemblingException("Syntax error", line);
        }
        
        this->rawOperand1 = operands;
        this->operand1 = new Operand(operands);

        if (!this->operand1->isValid()) {
            throw AssemblingException("Invalid operand", line);
        }
    }

    
}


Instruction::~Instruction() {
    if (operand1 != nullptr) {
        delete operand1;
        operand1 = nullptr;
    }
    
    if (operand2 != nullptr) {
        delete operand2;
        operand2 = nullptr;
    }
}