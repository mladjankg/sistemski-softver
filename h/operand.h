#ifndef _SS_OPERAND_H_
#define _SS_OPERAND_H_
#include <string>

namespace ss {
    class Operand {
    public:
        Operand(std::string);

        bool isValid() const { return valid; }

        bool requiresExtraBytes() const { return extraBytes; }
    private:
        std::string text;
        bool valid;
        bool extraBytes;
    };
}
#endif