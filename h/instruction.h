#ifndef _SS_INSTRUCTION_H_
#define _SS_INSTRUCTION_H_
#include <string>
#include <regex>
#include "asm_declarations.h"



namespace ss {
    class Operand;
    
    class Instruction {
    public:
        Instruction() : size(2) {}

        void parseInstruction(std::string, int);

        Operand* getOperand1() const {
            return operand1;
        }

        Operand* getOperand2() const {
            return operand2;
        }

        ~Instruction();
    private:
        static std::regex al, eq, ne, gt, unc;

        Operand* parseOperand(std::string) throw();
        
        InstructionCode instruction;
        AddressingCode addressing;
        ConditionCode condition;

        std::string rawOperand1;
        Operand* operand1;    

        std::string rawOperand2;
        Operand* operand2;

        size_t size;
    };
}

#endif