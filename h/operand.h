#ifndef _SS_OPERAND_H_
#define _SS_OPERAND_H_
#include <string>
#include <regex>
#include "asm_declarations.h"

namespace ss {
    class Assembler;
    
    class Operand {
    public:

        Operand(const std::string);

        Operand(Operand&) = delete;
        Operand(Operand&&) = delete;
        
        bool isValid() const { return valid; }

        bool requiresExtraBytes() const { return extraBytes; }

        const OperandType getType() const { return this->type; }
        
        const AddressingCode getAddressing() const { return this->addressing; }

        const std::string& getRawText() const { return this->text; }

        void setAddressing(const AddressingCode a) { this->addressing = a; }
    
    protected:
        friend class Assembler;
    private:
        
        static std::regex decimalRegex;
        static std::regex regIndDecRegex;
        static std::regex regIndLabRegex;
        static std::regex regDirRegex;
        static std::regex labelRegex;
        static std::regex locationRegex;

        std::string text;
        OperandType type;
        AddressingCode addressing;
        bool valid;
        bool extraBytes;
    };
}
#endif