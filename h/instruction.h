#ifndef _SS_INSTRUCTION_H_
#define _SS_INSTRUCTION_H_
#include <string>
#include <regex>
#include "asm_declarations.h"



namespace ss {
    class Operand;
    
    class Instruction {
    public:
        Instruction() {}

        void parseInstruction(std::string) throw();

        ~Instruction();
    private:
        static std::regex al, eq, ne, gt, unc;

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