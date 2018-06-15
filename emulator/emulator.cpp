#include "emulator.h"
#include "asm_declarations.h"
#include "ss_exceptions.h"
#include "instruction.h"
using namespace ss;

void Emulator::startEmulation() {
    this->running = true;
    this->run();
}

void Emulator::run() {
    while (running) {
        this->fetchInstruction();
        this->getOperands();
    }
}

void Emulator::fetchInstruction() {
    //Reading first two bytes of instruction
    //C++ by default reads data as little endian and swaps bytes, so we need to swap it back.
    Address firstHalf = this->getMemoryValue(this->memory + cpu.r[PC]);
    firstHalf = this->swapBytes(firstHalf);

    //Incrementing PC
    cpu.r[PC] += 2;

    //Moving bytes to ir register
    cpu.ir0 = firstHalf;

    //Fetching opcode and checking it.
    char op = (firstHalf & OPCODE_MASK) >> OPCODE_SHIFT;
    InstructionCode opCode = (InstructionCode)op;

    if (!this->opCodeValid(opCode)) {
        throw EmulatingException("Invalid op code, opCode = " + opCode);
    }

    if (Instruction::operandNumber[opCode] == 0) {
        return;
    }

    if (Instruction::operandNumber[opCode] == 1) {
        switch (opCode) {
            //Push and pop are specific because they read first operand from src reg
            case PUSH:
            case CALL: {
                char add = (firstHalf & OP2_ADDR) >> OP2_ADDR_SHIFT;
                AddressingCode addressing = (AddressingCode)add;

                if (!this->addressingValid(addressing)) {
                    throw EmulatingException("Invalid addressing code, opCode = " + opCode);
                }

                //r0-r7 or psw
                bool isPsw = (addressing == IMMED) && ((firstHalf & OP2_REG) == 0x7);
                if ((addressing == REGDIR) || isPsw) {
                    return;
                }

                else {
                    //Reading first two bytes of instruction
                    Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC]);
                    //secondHalf = this->swapBytes(secondHalf);

                    //Incrementing PC
                    cpu.r[PC] += 2;
                    cpu.ir1 = secondHalf;                    
                }
                return;
            }
            //For instructions that read first operand from dst 
            default: {
                char add = (firstHalf & OP1_ADDR) >> OP1_ADDR_SHIFT;
                AddressingCode addressing = (AddressingCode)add;

                if (!this->addressingValid(addressing)) {
                    throw EmulatingException("Invalid addressing code, addressingCode = " + addressing);
                }
                bool isPsw = (addressing == IMMED) && (((firstHalf & OP1_REG) >> OP1_REG_SHIFT) == 0x7);
                if ((addressing == REGDIR) || isPsw) {
                    return;
                }          
                else {
                    //Reading first two bytes of instruction
                    Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC]);
                    //secondHalf = this->swapBytes(secondHalf);

                    //Incrementing PC
                    cpu.r[PC] += 2;
                    cpu.ir1 = secondHalf;                    
                }
                return;
            }
        }
    }
    else {
        //Instructions that have two operands
        char add1 = (firstHalf & OP1_ADDR) >> OP1_ADDR_SHIFT;
        AddressingCode addressing1 = (AddressingCode)add1;

        if (!this->addressingValid(addressing1)) {
            throw EmulatingException("Invalid addressing code, addressingCode = " + addressing1);
        }

        bool hasSecond = false;
        bool isPsw1 = (addressing1 == IMMED) && (((firstHalf & OP1_REG) >> OP1_REG_SHIFT) == 0x7);
        if (!(addressing1 == REGDIR) && !isPsw1) {
        
            //Reading seocnd two bytes of instruction
            Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC]);
            //secondHalf = this->swapBytes(secondHalf);

            //Incrementing PC
            cpu.r[PC] += 2;

            cpu.ir1 = secondHalf;              
            hasSecond = true;                    
        }
        
        char add2 = (firstHalf & OP2_ADDR) >> OP2_ADDR_SHIFT;
        AddressingCode addressing2 = (AddressingCode)add2;

        if (!this->addressingValid(addressing2)) {
            throw EmulatingException("Invalid addressing code, addressingCode = " + addressing2);
        }
        bool isPsw2 = (addressing2 == IMMED) && ((firstHalf & OP2_REG) == 0x7);
        if (!(addressing2 == REGDIR) && !isPsw2) {
        
            if (hasSecond) {
                throw EmulatingException("Found combination of two memory addresing in one instruction.");
            }
            //Reading second two bytes of instruction
            Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC]);
            //secondHalf = this->swapBytes(secondHalf);

            //Incrementing PC
            cpu.r[PC] += 2;
            cpu.ir1 = secondHalf;
            hasSecond = true;                    
        }

        return;
    }
}

void Emulator::getOperands() {
    
    InstructionCode opCode = (InstructionCode)((cpu.ir0 & OPCODE_MASK) >> OPCODE_SHIFT);

    if (Instruction::operandNumber[opCode] == 0) {
        return;
    }

    if (Instruction::operandNumber[opCode] == 1) {
        switch (opCode) {
            case PUSH:
            case CALL: {
                char add = (cpu.ir0 & OP2_ADDR) >> OP2_ADDR_SHIFT;
                AddressingCode addressing = (AddressingCode)add;

                if (!this->addressingValid(addressing)) {
                    throw EmulatingException("Invalid addressing code, addressingCode = " + addressing);
                }

                this->fetchOperand(cpu.src, OP2_ADDR, OP2_ADDR_SHIFT, OP2_REG, OP2_REG_SHIFT, opCode, addressing);
                break;
            }

            default: {
                char add = (cpu.ir0 & OP1_ADDR) >> OP1_ADDR_SHIFT;
                AddressingCode addressing = (AddressingCode)add;

                if (!this->addressingValid(addressing)) {
                    throw EmulatingException("Invalid addressing code, addressingCode = " + addressing);
                }

                this->fetchOperand(cpu.dst, OP1_ADDR, OP1_ADDR_SHIFT, OP1_REG, OP1_REG_SHIFT, opCode, addressing);

                return;
            }
        }
    }
    else {
        //Getting first operand
        char add1 = (cpu.ir0 & OP1_ADDR) >> OP1_ADDR_SHIFT;
        AddressingCode addressing1 = (AddressingCode)add1;

        if (!this->addressingValid(addressing1)) {
            throw EmulatingException("Invalid addressing code, addressingCode = " + addressing1);
        }

        this->fetchOperand(cpu.dst, OP1_ADDR, OP1_ADDR_SHIFT, OP1_REG, OP1_REG_SHIFT, opCode, addressing1);

        //Getting second operand
        char add2 = (cpu.ir0 & OP2_ADDR) >> OP2_ADDR_SHIFT;
        AddressingCode addressing2 = (AddressingCode)add2;

        if (!this->addressingValid(addressing2)) {
            throw EmulatingException("Invalid addressing code, addressingCode = " + addressing2);
        }

        this->fetchOperand(cpu.src, OP2_ADDR, OP2_ADDR_SHIFT, OP2_REG, OP2_REG_SHIFT, opCode, addressing2);


        return;
    }
    
}

void Emulator::executeInstruction() {
    InstructionCode opCode = (InstructionCode)((cpu.ir0 & OPCODE_MASK) >> OPCODE_SHIFT);

    switch(opCode) {
        case ADD: 
        case SUB: 
        case MUL: 
        case DIV: {
            this->doArithmeticInstruction(opCode);
            break;
        }
        case CMP: {
            bool srcSign = (cpu.src & MOST_SIGNIFICANT_BIT);
            bool dstSign = (cpu.dst & MOST_SIGNIFICANT_BIT);
            
            short temp = cpu.dst - cpu.src;
            bool resSign = (temp & MOST_SIGNIFICANT_BIT);
            
            bool overflow = (!dstSign && srcSign && resSign) || (dstSign && !srcSign && !resSign);
            if (overflow) {
                cpu.psw = cpu.psw | SET_O;
            }
            else {
                cpu.psw = cpu.psw & RESET_O;
            }

            bool carry = (!dstSign && srcSign && !resSign) || (dstSign && srcSign && resSign) || (dstSign && !srcSign && resSign);
            if (carry) {
                cpu.psw = cpu.psw |  SET_C;
            }
            else {
                cpu.psw = cpu.psw & RESET_C;
            }
            break;
        }
        case AND:
        case OR:
        case NOT:
        case TEST: {
            this->doLogicInstruction(opCode);
            break;
        }
        case PUSH: {
            cpu.r[SP] -= 2;
            if (cpu.r[SP] < this->stackStart - this->stackSize) {
                throw EmulatingException("Stack overflow.");
            }

            this->setMemoryValue(this->memory + cpu.r[SP], cpu.src);
            break;
        }
        case POP: {
            if (cpu.r[SP] > this->stackStart) {
                throw EmulatingException("Memory access violation.");
            }
            cpu.dst = this->getMemoryValue(this->memory + cpu.r[SP]);
            cpu.r[SP] += 2;
            break;
        }
        case CALL: {
            break;
        }
        case IRET: {
            if (cpu.r[SP] > this->stackStart) {
                throw EmulatingException("Memory access violation.");
            }
            cpu.psw = this->getMemoryValue(this->memory + cpu.r[SP]);
            cpu.r[SP] += 2;

            if (cpu.r[SP] > this->stackStart) {
                throw EmulatingException("Memory access violation.");
            }

            cpu.r[PC] = this->getMemoryValue(this->memory + cpu.r[SP]);
            cpu.r[SP] += 2;
            break;
        }
        case MOV: {
            cpu.dst = cpu.src;
            this->setZN();
            break;
        }
        case SHL: {
            break;
        }
        case SHR: {
            break;
        }
    }
}
bool Emulator::opCodeValid(const InstructionCode opCode) const {
    return (opCode >= InstructionCode::ADD) && (opCode <= InstructionCode::SHR);
}

bool Emulator::addressingValid(const AddressingCode addCode) const {
    return (addCode >= AddressingCode::IMMED) && (addCode <= AddressingCode::REGINDPOM);
}

Address Emulator::swapBytes(const Address bytes) const {
    char firstHalf = (bytes >> 8) & 0xFF;
    char secondHalf = bytes & 0xFF;

    unsigned short newHigh = (unsigned short)secondHalf << 8;
    unsigned short newLow = (unsigned short)firstHalf & 0xFF;
    return newHigh | newLow;
}

Address Emulator::getMemoryValue(char* addr) const {
    Address* mar = (Address*)addr;

    
    return *mar;
}

void Emulator::fetchOperand(short& writeReg, short opAddr, short opAddrShift, short opReg, short opRegShift, InstructionCode opCode, AddressingCode addressing) {

    switch (addressing) {
        case REGDIR: {
            char reg = 0;
            if (opRegShift) {
                reg = (cpu.ir0 & opReg) >> opRegShift;
            }
            else {
                reg = cpu.ir0 & opReg;
            }
            writeReg = cpu.r[reg];
            break;
        }

        case MEMDIR: {
            Address val = this->getMemoryValue(this->memory + cpu.ir1);
            writeReg = val;
            break;
        }

        case REGINDPOM: {
            char reg = 0;
            if (opRegShift) {
                reg = (cpu.ir0 & opReg) >> opRegShift;
            }
            else {
                reg = cpu.ir0 & opReg;
            }
            Address val = this->getMemoryValue(this->memory + cpu.ir1 + cpu.r[reg]);
            writeReg = val;
            break;
        }

        case IMMED: {
            //checking if argument is psw
            bool psw = false;
            if (opRegShift) {
                psw = ((cpu.ir0 & opReg) >> opRegShift) == 0x7;
            }
            else {
                psw = (cpu.ir0 & opReg) == 0x7;
            }

            if (psw  && (opCode == CALL)) {
                throw AssemblingException("Cannot use psw register with call instruciton.");
            }

            else if (psw) {
                writeReg = cpu.psw;
            }

            else {
                writeReg = cpu.ir1;
            }
            break;
        }
    }
}

void Emulator::doLogicInstruction(InstructionCode opCode) {
    switch(opCode) {
        case AND: {
            cpu.dst &= cpu.src;
            break;
        }
        case OR: {
            cpu.dst |= cpu.src;
            break;
        }
        case NOT: {
            cpu.dst = ~cpu.src;
            break;
        }
        case TEST: {
            Address temp = cpu.dst & cpu.src;
            
            if (temp == 0) {
                cpu.psw = cpu.psw | SET_Z;
            }
            else {
                cpu.psw = cpu.psw & RESET_Z;
            }

            if (temp & MOST_SIGNIFICANT_BIT) {
                cpu.psw = cpu.psw | SET_N;
            }
            else {
                cpu.psw = cpu.psw & RESET_N;
            }

            break;
        }
        default: {
            this->invalidInstruciton();
            return;
        }
        
        if (opCode != TEST) {
            this->setZN();
        } 
    }
}

void Emulator::doArithmeticInstruction(InstructionCode opCode) {
    
    bool srcSign = (cpu.src & MOST_SIGNIFICANT_BIT);
    bool dstSign = (cpu.dst & MOST_SIGNIFICANT_BIT);

    switch(opCode) {
        case ADD: {
            cpu.dst += cpu.src;
            break;
        }           
        case SUB: {
            cpu.dst -= cpu.src;
            break;
        }
        case MUL: {
            cpu.dst *= cpu.src;
            break;
        }
        case DIV: {
            cpu.dst /= cpu.src;
            break;
        }
    }

    bool resSign = cpu.dst & MOST_SIGNIFICANT_BIT;

    this->setZN();
    if (opCode == ADD) {
        bool overflow = (dstSign && srcSign && !resSign) || (!dstSign && !srcSign && resSign);
        if (overflow) {
            cpu.psw = cpu.psw | SET_O;
        }
        else {
            cpu.psw = cpu.psw & RESET_O;
        }

        bool carry = (dstSign && srcSign) || (!dstSign && srcSign && !resSign) || (dstSign && !srcSign && !resSign);
        if (carry) {
            cpu.psw = cpu.psw |  SET_C;
        }
        else {
            cpu.psw = cpu.psw & RESET_C;
        }
        
    }
    else if(opCode == SUB) {
        bool overflow = (!dstSign && srcSign && resSign) || (dstSign && !srcSign && !resSign);
        if (overflow) {
            cpu.psw = cpu.psw | SET_O;
        }
        else {
            cpu.psw = cpu.psw & RESET_O;
        }

        bool carry = (!dstSign && srcSign && !resSign) || (dstSign && srcSign && resSign) || (dstSign && !srcSign && resSign);
        if (carry) {
            cpu.psw = cpu.psw |  SET_C;
        }
        else {
            cpu.psw = cpu.psw & RESET_C;
        }
    }
}

void Emulator::doShift(InstructionCode opCode) {
    if (opCode == SHL) {
        bool carry = 
    }
}

void Emulator::invalidInstruciton() {

}

void Emulator::setZN() {
    if (cpu.dst == 0) {
        cpu.psw = cpu.psw | SET_Z;
    }
    else {
        cpu.psw = cpu.psw & RESET_Z;
    }
    if (cpu.dst & MOST_SIGNIFICANT_BIT) {
        cpu.psw = cpu.psw | SET_N;
    }
    else {
        cpu.psw = cpu.psw & RESET_N;
    }
}




// ADD
//        A                   B                   A + B              Flags  
//  ---------------     ----------------    ---------------      -----------------
//  h  |  ud  |   d   | h  |  ud  |   d   | h  |  ud  |   d   | OF | SF | ZF | CF
//  ---+------+-------+----+------+-------+----+------+-------+----+----+----+---
//  7F | 127  |  127  | 0  |  0   |   0   | 7F | 127  |  127  | 0  | 0  | 0  | 0
//  FF | 255  |  -1   | 7F | 127  |  127  | 7E | 126  |  126  | 0  | 0  | 0  | 1
//  0  |  0   |   0   | 0  |  0   |   0   | 0  |  0   |   0   | 0  | 0  | 1  | 0
//  FF | 255  |  -1   | 1  |  1   |   1   | 0  |  0   |   0   | 0  | 0  | 1  | 1
//  FF | 255  |  -1   | 0  |  0   |   0   | FF | 255  |  -1   | 0  | 1  | 0  | 0
//  FF | 255  |  -1   | FF | 255  |  -1   | FE | 254  |  -2   | 0  | 1  | 0  | 1
//  FF | 255  |  -1   | 80 | 128  | -128  | 7F | 127  |  127  | 1  | 0  | 0  | 1
//  80 | 128  | -128  | 80 | 128  | -128  | 0  |  0   |   0   | 1  | 0  | 1  | 1
//  7F | 127  |  127  | 7F | 127  |  127  | FE | 254  |  -2   | 1  | 1  | 0  | 0


//  SUB
//        A                   B                   A - B              Flags  
//  ---------------     ----------------    ---------------      -----------------
//  h  |  ud  |   d   | h  |  ud  |   d   | h  |  ud  |   d   || OF | SF | ZF | CF
// ----+------+-------+----+------+-------+----+------+-------++----+----+----+----
//  FF | 255  |  -1   | FE | 254  |  -2   | 1  |  1   |   1   || 0  | 0  | 0  | 0
//  7E | 126  |  126  | FF | 255  |  -1   | 7F | 127  |  127  || 0  | 0  | 0  | 1
//  FF | 255  |  -1   | FF | 255  |  -1   | 0  |  0   |   0   || 0  | 0  | 1  | 0
//  FF | 255  |  -1   | 7F | 127  |  127  | 80 | 128  | -128  || 0  | 1  | 0  | 0
//  FE | 254  |  -2   | FF | 255  |  -1   | FF | 255  |  -1   || 0  | 1  | 0  | 1
//  FE | 254  |  -2   | 7F | 127  |  127  | 7F | 127  |  127  || 1  | 0  | 0  | 0
//  7F | 127  |  127  | FF | 255  |  -1   | 80 | 128  | -128  || 1  | 1  | 0  | 1