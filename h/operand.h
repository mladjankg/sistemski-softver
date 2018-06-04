#ifndef _SS_OPERAND_H_
#define _SS_OPERAND_H_
#include <string>
#include <regex>

namespace ss {
    class Operand {
    public:
        Operand(std::string&);

        Operand(Operand&) = delete;
        Operand(Operand&&) = delete;
        
        bool isValid() const { return valid; }

        bool requiresExtraBytes() const { return extraBytes; }

        enum OperandType {
            IMMED_VAL,              // 20 – immediate value 20
            LABEL_VAL,              // &x – value of symbol x
            MEMDIR_VAL,             // x – memory direct addressing
            DECIMAL_LOCATION_VAL,   // *20 – location from addres 20
            REGDIR_VAL,             // r3 – register direct
            REGIND_DEC_VAL,         // r4[32] – register indirect with immediate offset
            REGIND_LAB_VAL,         // r5[x] – register indirect with variable offset
            PCREL_VAL               // $x – PC relative addresing of variable x
        };

        OperandType getType() const { return this->type; }
        
        const std::string& getRawText() const { return this->text; }
    private:
        
        static std::regex decimalRegex;
        static std::regex regIndDecRegex;
        static std::regex regIndLabRegex;
        static std::regex regDirRegex;
        static std::regex labelRegex;
        static std::regex locationRegex;

        std::string text;
        OperandType type;
        bool valid;
        bool extraBytes;
    };
}
#endif