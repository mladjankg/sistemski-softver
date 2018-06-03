#ifndef _SS_INSTRUCTION_H_
#define _SS_INSTRUCTION_H_
#include <string>
#include <regex>
#include "asm_declarations.h"

namespace ss {
    class Instruction {
    public:
        Instruction() {}

        void parseInstruction(std::string) throw();
    private:
        static std::regex al, eq, ne, gt, unc;

        InstructionCode instruction;
        AddressingCode addressing;
        ConditionCode condition;

        std::string operand1;
        std::string operand2;

        size_t size;
    };
}

#endif