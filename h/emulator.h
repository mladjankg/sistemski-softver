#ifndef _SS_EMULATOR_
#define _SS_EMULATOR_

#include "asm_declarations.h"
#include "executable.h"

#define PC 7
#define SP 8
#define OPCODE_MASK 0x3C00
#define OPCODE_SHIFT 10
#define OP1_ADDR 0x0300
#define OP1_ADDR_SHIFT 8
#define OP2_ADDR 0x0018
#define OP2_ADDR_SHIFT 3
#define OP2_REG 0x0007

namespace ss {
typedef unsigned short Address;
    struct CPU {
        short r[8];
        short psw;
        short src;
        short dst;
        short alu;
        short ir0;
        short ir1;
    };


    class Emulator {
    public:
        Emulator(Executable e) {
            this->memory = (Address*)e.content;
            this->cpu.r[7] = e.startAddress;
        }

        Emulator(char* memory, Address start) : callStack(0), running(false) {
            this->cpu.r[7] = start;
            this->memory = (Address*)memory;
        }

        void startEmulation();

        bool isRunning() const { return this->running; }
    private:

        void fetchInstruction();
        void getOperands();
        void executeInstruction();

        bool opCodeValid(const InstructionCode opCode) const;
        bool addressingValid(const AddressingCode addCode) const;
        short swapBytes(const short bytes) const;
        short getMemoryValue(Address memoryLocation) const;

        CPU cpu;
        Address* memory;
        int callStack;
        bool running;
    };
}
#endif