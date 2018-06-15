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
#define OP1_REG 0x00E0
#define OP1_REG_SHIFT 5

#define OP2_ADDR 0x0018
#define OP2_ADDR_SHIFT 3
#define OP2_REG 0x0007
#define OP2_REG_SHIFT 0x0

#define RESET_Z 0xFFFE
#define SET_Z 0x0001
#define RESET_O 0xFFFD
#define SET_O 0x0002
#define RESET_C 0xFFFB
#define SET_C 0x0004
#define RESET_N 0xFFF7
#define SET_N 0x0008

#define NEGATIVE_MASK 0x8000
#define MOST_SIGNIFICANT_BIT 0x8000
namespace ss {
typedef unsigned short Address;
    struct CPU {
        Address r[8];
        Address psw;
        short src;
        short dst;
        Address alu;
        Address ir0;
        Address ir1;
    };


    class Emulator {
    public:
        Emulator(Executable e) : Emulator(e.content, e.startAddress) {
            //this->memory = e.content;
            //this->cpu.r[7] = e.startAddress;
        }

        Emulator(char* memory, Address start) : callStack(0), running(false) {
            this->cpu.r[7] = start;
            this->memory = memory;
            bool instructionError = false;
        }

        void startEmulation();
        void run();
        bool isRunning() const { return this->running; }
    private:

        void fetchInstruction();
        void getOperands();
        void executeInstruction();


        void fetchOperand(short& writeReg, short opAddr, short opAddrShift, short opReg, short opRegShift, InstructionCode opCode, AddressingCode addresing);
        void setZN();

        bool opCodeValid(const InstructionCode opCode) const;
        bool addressingValid(const AddressingCode addCode) const;

        void doLogicInstruction(InstructionCode opCode);
        void doArithmeticInstruction(InstructionCode opCode);
        void doShift(InstructionCode opCode);
        
        void invalidInstruciton();

        Address swapBytes(const Address bytes) const;

        Address getMemoryValue(char* memoryLocation) const;
        void setMemoryValue(char* memoryLocation, short& value);

        CPU cpu;
        char* memory;
        
        Address stackStart;
        Address stackSize;

        int callStack;
        bool running;

        bool instructionError;
    };
}
#endif