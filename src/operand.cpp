#include "operand.h"
#include "utils.h"
#include "asm_declarations.h"
#include <string>
#include <regex>
using namespace ss;

std::regex Operand::decimalRegex("(^(-)?[0-9]+$)");
std::regex Operand::regIndDecRegex("^r[0-7]\[[ \\t]*[0-9]+[ \\t]*\]$");
std::regex Operand::regIndLabRegex("^r[0-7]\[[ \\t]*[a-zA-Z_]\\w*[ \\t]*\]$");
std::regex Operand::regDirRegex("^r[0-7]$");
std::regex Operand::labelRegex("^(&|\\$)?[a-zA-Z_]\\w*$");
std::regex Operand::locationRegex("^\\*[0-9]+$");

Operand::Operand(const std::string op) : valid(true), text(op) {

    bool regDir = std::regex_match(op, regDirRegex);
    bool regIndLab = std::regex_match(op, regIndLabRegex);
    bool regIndDec = std::regex_match(op, regIndDecRegex);
    if (std::regex_match(op, labelRegex) && !regIndLab && !regIndDec && !regDir) {
        this->extraBytes = true;

        if (op[0] == '&') {
            this->type = OperandType::LABEL_VAL;
            this->addressing = AddressingCode::IMMED; 
        }
        
        else if (op[0] == '$') {
            this->type = OperandType::PCREL_VAL;
            this->addressing = AddressingCode::REGINDPOM;
        }

        else {
            this->type = OperandType::MEMDIR_VAL;
            this->addressing = AddressingCode::MEMDIR;
        }
    }

    else if (regDir) {
        this->extraBytes = false;
        this->type = OperandType::REGDIR_VAL;
        this->addressing = AddressingCode::REGDIR;
    }

    else if (regIndDec) {
        this->text = Utils::removeEmptySpaces(this->text);
        this->extraBytes = true;
        this->type = OperandType::REGIND_DEC_VAL;
        this->addressing = AddressingCode::REGINDPOM;

    }

    else if (regIndLab) {
        this->text = Utils::removeEmptySpaces(this->text);
        this->extraBytes = true;
        this->type = OperandType::REGIND_LAB_VAL;
        this->addressing = AddressingCode::REGINDPOM;
    }

    else if (std::regex_match(op, locationRegex)) {
        this->extraBytes = true;
        this->type = OperandType::DECIMAL_LOCATION_VAL;
        this->addressing = AddressingCode::MEMDIR;
    }

    else if (std::regex_match(op, decimalRegex)) {
        this->extraBytes = true;
        this->type = OperandType::IMMED_VAL;
        this->addressing = AddressingCode::IMMED;
    }

    else {
        this->valid = false;
    }
}