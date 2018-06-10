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

const char Instruction::operandNumber[] = {2, 2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 0, 2, 2, 2, 0, 1, 2};

void Instruction::parseInstruction(std::string line, int lineNumber) {
    

    size_t spPos = line.find_first_of(' ');
    std::string operands(Utils::empty);
    std::string mnemonic(Utils::empty);
    if (spPos == std::string::npos) {
        //ret, iret
        this->size = 2;
        mnemonic = line;

    }
    else {
        mnemonic = line.substr(0, spPos);
        operands = line.substr(spPos + 1);
    }
    
    //Parsing instruction condition.    
    bool sufix = false;
    if (std::regex_match(mnemonic, al)) {
        sufix = true;
        this->condition = ConditionCode::AL;
    }

    else if (std::regex_match(mnemonic, eq)) {
        sufix = true;
        this->condition = ConditionCode::EQ;
    }

    else if (std::regex_match(mnemonic, ne)) {
        sufix = true;
        this->condition = ConditionCode::NE;
    }

    else if (std::regex_match(mnemonic, gt)) {
        sufix = true;
        this->condition = ConditionCode::GT;
    }

    else if (std::regex_match(mnemonic, unc)) {
        this->condition = ConditionCode::AL;
    } 

    else {
        throw AssemblingException("Unknown instruction", line, lineNumber);
    }   
    
    if (sufix) {
        mnemonic = mnemonic.substr(0, mnemonic.length() - 2);
    }

    //Parsing instruction mnemonic
    std::transform(mnemonic.begin(), mnemonic.end(), mnemonic.begin(), ::tolower);

    bool instructionValid = false;

    switch(mnemonic.length()) {
        case 2: {
            if (mnemonic.compare("or") == 0) {
                this->instruction = InstructionCode::OR;
                instructionValid = true;
            }
            break;
        }
        case 3: {
            if (mnemonic.compare("mov") == 0) {
                this->instruction = InstructionCode::MOV;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("add") == 0) {
                this->instruction = InstructionCode::ADD;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("sub") == 0) {
                this->instruction = InstructionCode::SUB;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("pop") == 0) {
                this->instruction = InstructionCode::CMP;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("cmp") == 0) {
                this->instruction = InstructionCode::CMP;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("mul") == 0) {
                this->instruction = InstructionCode::MUL;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("div") == 0) {
                this->instruction = InstructionCode::DIV;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("and") == 0) {
                this->instruction = InstructionCode::AND;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("not") == 0) {
                this->instruction = InstructionCode::NOT;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("shl") == 0) {
                this->instruction = InstructionCode::SHL;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("shr") == 0) {
                this->instruction = InstructionCode::SHR;
                instructionValid = true;
            }

            else if (mnemonic.compare("jmp") == 0) {
                this->instruction = InstructionCode::JMP;
                instructionValid = true;
            }
            
            else if (mnemonic.compare("ret") == 0) {
                this->instruction = InstructionCode::RET;
                instructionValid = true;
            }

            break;
        }
        case 4: {
            if (mnemonic.compare("push") == 0) {
                this->instruction = InstructionCode::PUSH;
                instructionValid = true;
            }
            else if (mnemonic.compare("test") == 0) {
                this->instruction = InstructionCode::TEST;
                instructionValid = true;
            }
            else if (mnemonic.compare("call") == 0) {
                this->instruction = InstructionCode::CALL;
                instructionValid = true;
            }
            else if (mnemonic.compare("iret") == 0) {
                this->instruction = InstructionCode::IRET;
                instructionValid = true;
            }
            break;
        }
    }

    if (!instructionValid) {
        throw AssemblingException("Unknown instruction", line, lineNumber);
    }
    
    //Parsing operands
    size_t commaPos;
    if ((commaPos = operands.find_first_of(',')) != std::string::npos) {
        //Parsing operands
        StringTokenizer st(",");
        st.tokenize(operands);

        if (st.tokenNumber() != Instruction::operandNumber[this->instruction]) {
            throw AssemblingException("Invalid argument number", line, lineNumber);
        }
        
        //Parsing first operand.
        this->rawOperand1 = st.nextToken();
        this->rawOperand1 = Utils::trim(this->rawOperand1);
        if (this->rawOperand1.find_first_of(Utils::emptyChars) != std::string::npos) {
            throw AssemblingException("Syntax error", line, lineNumber);
        }
        
        this->operand1 = new Operand(this->rawOperand1);

        if (!this->operand1->isValid()) {
            throw AssemblingException("Invalid operand", line, lineNumber);
        }
        
        if (this->operand1->requiresExtraBytes()) {
            this->size = 4;
        }
        
        //Parsing second operand.
        this->rawOperand2 = st.nextToken();
        this->rawOperand2 = Utils::trim(this->rawOperand2);
        if (this->rawOperand2.find_first_of(Utils::emptyChars) != std::string::npos) {
            throw AssemblingException("Syntax error", line, lineNumber);
        }
        
        this->operand2 = new Operand(this->rawOperand2);

        if (!this->operand2->isValid()) {
            throw AssemblingException("Invalid operand", line, lineNumber);
        }
        
        if (this->operand2->requiresExtraBytes() && (this->size == 2)) {
            this->size = 4;
        }
        
        else if (this->operand2->requiresExtraBytes() && (this->size == 4)) {
            throw AssemblingException("Invalid combinaton of operand addressing.", line);
        }
    }
    else {
        //Checking if operands are passed, and if instruction accepts operands.
        if (operands.find_first_not_of(Utils::emptyChars) == std::string::npos) {
            
        
            if (0 == Instruction::operandNumber[this->instruction]) {

                //Instruction ret is pseudo instruction that translates into pop pc.
                if (this->instruction == InstructionCode::RET) {
                    this->instruction = InstructionCode::POP;
                    this->operand1 = new Operand("r7"); //r7 <=> PC
                }
                
                return;
            }
            else {
                throw AssemblingException("Invalid argument number", line, lineNumber);
            }
        }
        operands = Utils::trim(operands);
        
        if (operands.find_first_of(Utils::emptyChars) != std::string::npos) {
            throw AssemblingException("Syntax error", line, lineNumber);
        }
        
        this->rawOperand1 = operands;
        this->operand1 = new Operand(operands);

        if (!this->operand1->isValid()) {
            throw AssemblingException("Invalid operand", line, lineNumber);
        }
        
        if (this->operand1->requiresExtraBytes()) {
            this->size = 4;
        }

        //JMP is pseudo instruction that translates into add or mov depending on addressing.
        if (this->instruction == InstructionCode::JMP) {
            this->operand2 = this->operand1;
            if (this->operand2->getType() == OperandType::MEMDIR_VAL) {
                this->instruction = InstructionCode::ADD_JMP;
            }
            else {
                this->instruction = InstructionCode::MOV;
            }

            this->operand1 = new Operand(std::string("r7")); //r7 <=> PC
        }


    }

    
}

Operand* Instruction::parseOperand(std::string op) throw() {
    return nullptr;
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