#ifndef _SS_INSTRUCTION_H_
#define _SS_INSTRUCTION_H_
#include <string>
#include <regex>
#include "asm_declarations.h"



namespace ss {
    class Operand;
    
    class Instruction {
    public:
        Instruction() : size(2), operand1(nullptr), operand2(nullptr) {}
        Instruction(size_t size) 
            : operand1(nullptr), operand2(nullptr), instruction(InstructionCode::ALIGN_INST), size(size) {}
        void parseInstruction(std::string, int);

        Operand* getOperand1() const {
            return this->operand1;
        }

        Operand* getOperand2() const {
            return this->operand2;
        }

        const size_t getInstructionSize() const {
            return this->size;
        }

        const ConditionCode getCondition() const {
            return this->condition;
        }

        const InstructionCode getInstruciton() const {
            return this->instruction;
        }



        static const char operandNumber[20]; 

        ~Instruction();
    private:
        static std::regex al, eq, ne, gt, unc;

        Operand* parseOperand(std::string) throw();
        
        InstructionCode instruction;

        ConditionCode condition;

        std::string rawOperand1;
        Operand* operand1;    

        std::string rawOperand2;
        Operand* operand2;

        size_t size;
        

    };
}

#endif