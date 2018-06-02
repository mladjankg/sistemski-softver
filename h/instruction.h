#ifndef _SS_INSTRUCTION_H_
#define _SS_INSTRUCTION_H_
#include <string>
#include "asm_declarations.h"

namespace ss {
    class Instruction {
    public:
        Instruction() {}

        bool parseInstruction(std::string);
    private:
        
        InstructionCode instruction;
        AddressingCode addressing;
        ConditionCode condition;

        std::string operand1;
        std::string operand2;
    };
}

#endif