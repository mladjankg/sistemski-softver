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
                AddressingCode addressing = (AddressingCode)((cpu.ir0 & OP2_ADDR) >> OP2_ADDR_SHIFT);
                switch (addressing) {
                    case REGDIR: {
                        char reg = (cpu.ir0 & OP2_REG);
                        cpu.src = cpu.r[reg];
                        break;
                    }

                    case MEMDIR: {
                        break;
                    }

                    case REGINDPOM: {
                        break;
                    }

                    case IMMED: {
                        char psw = (cpu.ir0 & OP2_REG);
                        if ((psw == 0x7) && (opCode == CALL)) {
                            throw AssemblingException("Cannot use psw register with call instruciton.");
                        }

                        short val = this->swapBytes(cpu.ir1);
                        cpu.src = val;
                    }
                }
            }
            default: {
                // char add = (firstHalf & OP1_ADDR) >> OP1_ADDR_SHIFT;
                // AddressingCode addressing = (AddressingCode)add;

                // if (!this->addressingValid(addressing)) {
                //     throw EmulatingException("Invalid addressing code, addressingCode = " + addressing);
                // }

                // if (addressing != REGDIR) {
                //     Address secondHalf = this->memory[cpu.r[PC]];
                //     cpu.ir1 = secondHalf;                    
                // }
                return;
            }
        }
    }
    else {
        // char add1 = (firstHalf & OP1_ADDR) >> OP1_ADDR_SHIFT;
        // AddressingCode addressing1 = (AddressingCode)add1;

        // if (!this->addressingValid(addressing1)) {
        //     throw EmulatingException("Invalid addressing code, addressingCode = " + addressing1);
        // }

        // bool hasSecond = false;
        // if (addressing1 != REGDIR) {
        //     Address secondHalf = this->memory[cpu.r[PC]];
        //     cpu.ir1 = secondHalf;
        //     hasSecond = true;                    
        // }
        
        // char add2 = (firstHalf & OP2_ADDR) >> OP2_ADDR_SHIFT;
        // AddressingCode addressing2 = (AddressingCode)add2;

        // if (!this->addressingValid(addressing2)) {
        //     throw EmulatingException("Invalid addressing code, addressingCode = " + addressing2);
        // }

        // if (addressing2 != REGDIR) {
        //     if (hasSecond) {
        //         throw EmulatingException("Found combination of two memory addresing in one instruction.");
        //     }
        //     Address secondHalf = this->memory[cpu.r[PC]];
        //     cpu.ir1 = secondHalf;
        //     hasSecond = true;                    
        // }

        // return;
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