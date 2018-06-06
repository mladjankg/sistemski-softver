#include "operand.h"
#include <string>
#include <regex>
using namespace ss;

std::regex Operand::decimalRegex("(^(-)?[0-9]+$)");
std::regex Operand::regIndDecRegex("^r[0-7]\[[0-9]+\]$");
std::regex Operand::regIndLabRegex("^r[0-7]\[[a-zA-Z_]\\w*\]$");
std::regex Operand::regDirRegex("^r[0-7]$");
std::regex Operand::labelRegex("^(&|\\$)?[a-zA-Z_]\\w*$");
std::regex Operand::locationRegex("^\\*[0-9]+$");

Operand::Operand(std::string& op) : valid(true), text(op) {
    bool regDir = std::regex_match(op, regDirRegex);
    bool regIndLab = std::regex_match(op, regIndLabRegex);
    bool regIndDec = std::regex_match(op, regIndDecRegex);
    if (std::regex_match(op, labelRegex) && !regIndLab && !regIndDec && !regDir) {
        this->extraBytes = true;

        if (op[0] == '&') {
            this->type = OperandType::LABEL_VAL;
        }
        
        else if (op[0] == '$') {
            this->type = OperandType::PCREL_VAL;
        }

        else {
            this->type = OperandType::MEMDIR_VAL;
        }
    }

    else if (regDir) {
        this->extraBytes = false;
        this->type = OperandType::REGDIR_VAL;
    }

    else if (regIndDec) {
        this->extraBytes = true;
        this->type = OperandType::REGIND_DEC_VAL;
    }

    else if (regIndLab) {
        this->extraBytes = true;
        this->type = OperandType::REGIND_LAB_VAL;
    }

    else if (std::regex_match(op, locationRegex)) {
        this->extraBytes = true;
        this->type = OperandType::DECIMAL_LOCATION_VAL;
    }

    else if (std::regex_match(op, decimalRegex)) {
        this->extraBytes = true;
        this->type = OperandType::IMMED_VAL;
    }

    else {
        this->valid = false;
    }
}