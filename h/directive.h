#ifndef _SS_DIRECTIVE_H_
#define _SS_DIRECTIVE_H_

#include "asm_declarations.h"
#include "operand.h"
#include "utils.h"
#include <list>

namespace ss {
    class Directive {
    public:
        Directive(DirectiveType type): type(type) {}
        
        DirectiveType getType() const {
            return type;
        }

        ~Directive();
    private:
        DirectiveType type;

    };

    class SkipDirective :public Directive {
    public:
        SkipDirective() : Directive(DirectiveType::SKIP), offset(0) {}

        void setOffset(unsigned int offset) {
            this->offset = offset;
        }

        const unsigned int getOffset() const { return this->offset; }

    private:
        unsigned int offset;
    };

    class BWLDirective :public Directive {
    public:
        BWLDirective(DirectiveType type): Directive(type) {}

        void setOperand(const std::string& op) {
            if (op.find_first_of(Utils::emptyChars) != std::string::npos) {
                throw 1;
            }

            this->operands.push_back(op);
        }
        
        const std::list<std::string>& getOperands() const {
            return operands;
        }

    private:

        std::list<std::string> operands;
    };
}
#endif /* _SS_DIRECTIVE_H_ */

