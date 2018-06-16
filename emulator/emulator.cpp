#include "emulator.h"
#include "asm_declarations.h"
#include "ss_exceptions.h"
#include "instruction.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <termios.h>
#include <unistd.h>
using namespace ss;



Emulator::Emulator(Executable* e) : callStack(0), running(false),
    stackStart(STACK_START), stackSize(STACK_SIZE) {
    this->cpu.r[7] = e->startAddress;
    this->memory = e->content;
    this->instructionError = false;
    #ifdef TIMER_INTERRUPT
    cpu.psw = 0 | TIMER_FLAG;
    #else
    cpu.psw = 0;
    #endif
    exe = e;
}

void Emulator::startEmulation() {
    

    struct termios t;
	tcgetattr(STDIN_FILENO, &t); //get the current terminal I/O structure
    t.c_lflag &= ~ICANON; //Manipulate the flag bits to do what you want it to do
    t.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t); //Apply the new settings


    cpu.psw = cpu.psw | SET_I;
    cpu.r[SP] = stackStart;
    this->running = true;

    std::thread timer(tick, this);
    std::thread kb(keyboard, this);
    try {
    //t.detach();
        this->run();
    }
    catch (std::exception& e) {
        std::cout << e.what();
        std::cout << std::endl << "Press any key to exit. ";
        mtx.lock();
        this->running = false;
        mtx.unlock();
    }

    timer.join();
    kb.join();
    
    tcgetattr(STDIN_FILENO, &t); //get the current terminal I/O structure
    t.c_lflag |= ICANON; //Manipulate the flag bits to do what you want it to do
    t.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &t); //Apply the new settings
}

void Emulator::tick(Emulator* emulator) {
    while(1) {
        std::this_thread::sleep_for(std::chrono::seconds(1));

        if (!emulator) break;
        emulator->mtx.lock();
        if (!emulator->running) {
             emulator->mtx.unlock();
             break;
        }


        emulator->mtx.unlock();
        emulator->registerInterrupt(TIMER);

    }
    return;
}

void Emulator::keyboard(Emulator* emulator) {
    bool running = true;
    while(1) {
        
        if (!emulator) break;
        emulator->mtx.lock();
        if (!emulator->running) {
            emulator->mtx.unlock();
            break;
        }
        emulator->mtx.unlock();
        char k;
		k = std::getchar();
        emulator->writeMtx.lock();
        if (emulator->running) {
            emulator->memory[KEYBOARD_REG] = k;
        }
        else {
            running = false;
        }
        emulator->writeMtx.unlock();
        if (running) 
            emulator->registerInterrupt(KEYBOARD);
        // Emulator::mtx.unlock();
    }
    return;
}

void Emulator::registerInterrupt(InterruptType type) {
    mtx.lock();
    this->interruptBuffer.push(type);
    mtx.unlock();
}
void Emulator::run() {
    while (running) {
        try {
            this->fetchInstruction();
            this->getOperands();
            this->executeInstruction();
        }
        catch (int& i) {
            this->instructionError = true;
        }
        this->interrupt();
        this->instructionError = false;
    }
    std::cout << "\nRun ended, press any key to exit. " << std::flush;
}

void Emulator::fetchInstruction() {
    //Reading first two bytes of instruction
    //C++ by default reads data as little endian and swaps bytes, so we need to swap it back.


    Address firstHalf = this->getMemoryValue(this->memory + cpu.r[PC], EX);
    firstHalf = this->swapBytes(firstHalf);

    //Incrementing PC
    cpu.r[PC] += 2;

    //Moving bytes to ir register
    cpu.ir0 = firstHalf;

    //Fetching opcode and checking it.
    char op = (firstHalf & OPCODE_MASK) >> OPCODE_SHIFT;
    InstructionCode opCode = (InstructionCode)op;

    if (!this->opCodeValid(opCode)) {
        this->instructionError = true;
        return;
        //throw EmulatingException("Invalid op code, opCode = " + opCode);
    }

    //Ako ovde izadjes pri dohvatanju instrukcije nece se dobro uvecati location counter za sledecu.
    // if (!(this->checkCondition())) {
    //     return;
    // }
    
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
                    std::cout << "Invalid addressing code, opCode = " << opCode;
                    this->instructionError = true;
                    return;
                    //throw EmulatingException("Invalid addressing code, opCode = " + opCode);
                }

                //r0-r7 or psw
                bool isPsw = (addressing == IMMED) && ((firstHalf & OP2_REG) == 0x7);
                if ((addressing == REGDIR) || isPsw) {
                    return;
                }

                else {

                    //Reading first two bytes of instruction
                    Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC], EX);
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
                    std::cout << "Invalid addressing code, addressingCode = " << addressing;
                    this->instructionError = true;
                    return;
                    //throw EmulatingException("Invalid addressing code, addressingCode = " + addressing);
                }
                bool isPsw = (addressing == IMMED) && (((firstHalf & OP1_REG) >> OP1_REG_SHIFT) == 0x7);
                if ((addressing == REGDIR) || isPsw) {
                    return;
                }          
                else {

                    //Reading first two bytes of instruction
                    Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC], EX);


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
            std::cout << "Invalid addressing code, addressingCode = " << addressing1;
            this->instructionError = true;
            return;

        }

        bool hasSecond = false;
        bool isPsw1 = (addressing1 == IMMED) && (((firstHalf & OP1_REG) >> OP1_REG_SHIFT) == 0x7);
        if (!(addressing1 == REGDIR) && !isPsw1) {

            //Reading seocnd two bytes of instruction
            Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC], EX);
            //secondHalf = this->swapBytes(secondHalf);

            //Incrementing PC
            cpu.r[PC] += 2;

            cpu.ir1 = secondHalf;              
            hasSecond = true;                    
        }
        
        char add2 = (firstHalf & OP2_ADDR) >> OP2_ADDR_SHIFT;
        AddressingCode addressing2 = (AddressingCode)add2;

        if (!this->addressingValid(addressing2)) {
            std::cout << "Invalid addressing code, addressingCode = " << addressing2;
            this->instructionError = true;
            return;
            //throw EmulatingException("Invalid addressing code, addressingCode = " + addressing2);
        }
        bool isPsw2 = (addressing2 == IMMED) && ((firstHalf & OP2_REG) == 0x7);
        if (!(addressing2 == REGDIR) && !isPsw2) {
        
            if (hasSecond) {
                std::cout<< "Found combination of two memory addresing in one instruction.";
                this->instructionError = true;
                return;
                //throw EmulatingException("Found combination of two memory addresing in one instruction.");
            }

            //Reading second two bytes of instruction
            Address secondHalf = this->getMemoryValue(this->memory + cpu.r[PC], EX);
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
    if (!this->checkCondition()) {
        return;
    }
    if (this->instructionError) {
        return;
    }
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
                    std::cout << "Invalid addressing code.";
                    this->instructionError = true;
                    return;
                    //throw EmulatingException("Invalid addressing code, addressingCode = " + addressing);
                }

                this->fetchOperand(cpu.src, OP2_ADDR, OP2_ADDR_SHIFT, OP2_REG, OP2_REG_SHIFT, opCode, addressing);
                break;
            }

            default: {
                char add = (cpu.ir0 & OP1_ADDR) >> OP1_ADDR_SHIFT;
                AddressingCode addressing = (AddressingCode)add;

                if (!this->addressingValid(addressing)) {
                    std::cout << "Invalid addressing code.";
                    this->instructionError = true;
                    return;
                    //throw EmulatingException("Invalid addressing code, addressingCode = " + addressing);
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
            std::cout << "Invalid addressing code.";
            this->instructionError = true;
            return;
            //throw EmulatingException("Invalid addressing code, addressingCode = " + addressing1);
        }

        this->fetchOperand(cpu.dst, OP1_ADDR, OP1_ADDR_SHIFT, OP1_REG, OP1_REG_SHIFT, opCode, addressing1);

        //Getting second operand
        char add2 = (cpu.ir0 & OP2_ADDR) >> OP2_ADDR_SHIFT;
        AddressingCode addressing2 = (AddressingCode)add2;

        if (!this->addressingValid(addressing2)) {
            std::cout << "Invalid addressing code.";
            this->instructionError = true;
            return;
            //throw EmulatingException("Invalid addressing code, addressingCode = " + addressing2);
        }

        this->fetchOperand(cpu.src, OP2_ADDR, OP2_ADDR_SHIFT, OP2_REG, OP2_REG_SHIFT, opCode, addressing2);


        return;
    }
    
}

void Emulator::executeInstruction() {
    if (!this->checkCondition()) {
        return;
    }
    if (this->instructionError) {
        return;
    }
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
            cpu.dst = this->getMemoryValue(this->memory + cpu.r[SP], RD);
            cpu.r[SP] += 2;
            break;
        }

        case CALL: {
            cpu.r[SP] -= 2;
            if (cpu.r[SP] < this->stackStart - this->stackSize) {
                throw EmulatingException("Stack overflow.");
            }
            short val = (short)cpu.r[PC];
            this->setMemoryValue(this->memory + cpu.r[SP], val);

            cpu.r[PC] = cpu.src;
            break;
        }

        case IRET: {
            if (cpu.r[SP] > this->stackStart) {
                throw EmulatingException("Memory access violation.");
            }
            cpu.psw = this->getMemoryValue(this->memory + cpu.r[SP], RD);
            cpu.r[SP] += 2;

            if (cpu.r[SP] > this->stackStart) {
                throw EmulatingException("Memory access violation.");
            }

            cpu.r[PC] = this->getMemoryValue(this->memory + cpu.r[SP], RD);
            cpu.r[SP] += 2;
            break;
        }

        case MOV: {
            cpu.dst = cpu.src;
            this->setZN();
            break;
        }

        case SHL: 
        case SHR: {
            this->doShift(opCode);
            break;
        }
    }

    if (!instructionError)
        this->storeOperand(opCode);
}

void Emulator::interrupt() {
    InstructionCode opCode = (InstructionCode)((cpu.ir0 & OPCODE_MASK) >> OPCODE_SHIFT);

    InterruptType type;
    if (!instructionError) {
        if (opCode == IRET) return;
        if (!(cpu.psw & SET_I)) {
            return; //Interrupt processing is disabled.
        }
        mtx.lock();
        if (this->interruptBuffer.size() == 0) {
            mtx.unlock();
            return; //No incomming interupts.
        }

        type = this->interruptBuffer.front();
        this->interruptBuffer.pop();
        mtx.unlock();    
        //Perserving current cpu state.
    }
    else {
        type = InterruptType::INSTR_ERR;
    }

    cpu.r[SP] -= 2;
    if (cpu.r[SP] < this->stackStart - this->stackSize) {
        throw EmulatingException("Stack overflow.");
    }

    short reg = (short)cpu.r[PC];
    this->setMemoryValue(this->memory + cpu.r[SP], reg);
    
    
    cpu.r[SP] -= 2;
    if (cpu.r[SP] < this->stackStart - this->stackSize) {
        throw EmulatingException("Stack overflow.");
    }

    reg = (short)cpu.psw;
    this->setMemoryValue(this->memory + cpu.r[SP], reg);
    

    Address nextPC = this->getMemoryValue(this->memory + 2 * type, RD);

    cpu.psw = cpu.psw & RESET_I;
    cpu.r[PC] = nextPC;
}

bool Emulator::opCodeValid(const InstructionCode opCode) const {
    return (opCode >= InstructionCode::ADD) && (opCode <= InstructionCode::SHR);
}

bool Emulator::addressingValid(const AddressingCode addCode) const {
    return (addCode >= AddressingCode::IMMED) && (addCode <= AddressingCode::REGINDPOM);
}

bool Emulator::checkCondition() const {
    char cond = (cpu.ir0 & CONDITION_MASK) >> CONDITION_SHIFT;
    ConditionCode condition = (ConditionCode)cond;

    switch (condition) {
        case EQ: {
            return cpu.psw & SET_Z;
            break;
        }
        case GT: {
            return !(cpu.psw & SET_N);
        }
        case NE: {
            return !(cpu.psw & SET_Z);
        }
        case AL:{
            return true;
        }
    }
}

Address Emulator::swapBytes(const Address bytes) const {
    char firstHalf = (bytes >> 8) & 0xFF;
    char secondHalf = bytes & 0xFF;

    unsigned short newHigh = (unsigned short)secondHalf << 8;
    unsigned short newLow = (unsigned short)firstHalf & 0xFF;
    return newHigh | newLow;
}

Address Emulator::getMemoryValue(char* addr, Access type) {
    // int address = reinterpret_cast<int>(addr);
    // Address shortAddress = (Address)(address & 0xFFFF);
    if (!this->access(addr - this->memory, type)) {
        this->instructionError = true;
        std::cout << "Segmentation fault.";
        throw 1;
    }
    writeMtx.lock();
    Address* mar = (Address*)addr;
    writeMtx.unlock();
    return *mar;
}

void Emulator::setMemoryValue(char* addr, short& val) {
    // int address = reinterpret_cast<int>(addr);
    // Address shortAddress = (Address)(address & 0xFFFF);
    if (!this->access(addr - this->memory, WR)) {
        std::cout << "Segmentation fault.";
        throw 1;
    }

    this->writeMtx.lock();
    Address* mar = (Address*)addr;
    *mar = val;


    int memAddr = (addr - this->memory);
    if (memAddr == OUTPUT_REG) {
        if (val == 0x10) {
            std::cout << ('\n') << std::flush;
        }
        else {
            std::cout << (char)val << std::flush;
        }
    }
    this->writeMtx.unlock();
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
            Address val = this->getMemoryValue(this->memory + cpu.ir1, RD);
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
            Address val = this->getMemoryValue(this->memory + cpu.ir1 + cpu.r[reg], RD);
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
                throw EmulatingException("Cannot use psw register with call instruciton.");
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

void Emulator::storeOperand(InstructionCode opCode) {

    switch (opCode) {
        case ADD: case SUB: case MUL: case DIV:
        case AND: case OR:  case NOT:
        case MOV: case SHR: case SHL: case POP: {
            char add = (cpu.ir0 & OP1_ADDR) >> OP1_ADDR_SHIFT;
            AddressingCode addressing = (AddressingCode)add;
            switch (addressing) {
                case REGDIR: {
                    char reg = 0;
                    reg = (cpu.ir0 & OP1_REG) >> OP1_REG_SHIFT;

                    bool halt = ((reg == 7) && ((cpu.dst == (short)MAX_SHORT)));
                    if (halt) {
                        Emulator::mtx.lock();
                        this->running = false;
                        Emulator::mtx.unlock();
                    }
                        
                    cpu.r[reg] = cpu.dst;

                    
                    break;
                }

                case MEMDIR: {
                    this->setMemoryValue(this->memory + cpu.ir1, cpu.dst);

                    break;
                }

                case REGINDPOM: {
                    char reg = 0;
                    reg = (cpu.ir0 & OP1_REG) >> OP1_REG_SHIFT;
                    this->setMemoryValue(this->memory + cpu.ir1 + cpu.r[reg], cpu.dst);
                    break;
                }

                case IMMED: {
                    this->invalidInstruciton();
                    break;
                }
            }
            break;
        }
        default:
            break;
    }
    return;
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
        
        
    }
    if (opCode != TEST) {
        this->setZN();
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

    bool carry = false;
    for (int i = 0; i < (Address)cpu.src && i < 16; ++i) {
        if (opCode == SHL) {
            carry = (cpu.dst & MOST_SIGNIFICANT_BIT);
            cpu.dst <<= 1;
        }
        else {
            carry = (cpu.dst & LEAST_SIGNIFICANT_BIT);
            cpu.dst >>= 1;
        }
    }

    this->setZN();
    if (carry) {
        cpu.psw = cpu.psw | SET_C;
    }
    else {
        cpu.psw = cpu.psw & RESET_C;
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

bool Emulator::access(Address address, Access type) {
    bool found = false;
    if (type == EX) {
        
        for(int i = 0; (i < exe->ex.size()) && !found; ++i) {
            found = (exe->ex[i].low <= address) && (address <= exe->ex[i].high);    
        }

        return found;
    }

    if (type == WR || type == RD) {
        for(int i = 0; i < exe->rw.size(); ++i) {
            if ((exe->rw[i].low <= address) && (address <= exe->rw[i].high)) {
                return true;
            }
        }
        if (type == WR)
            return false;
    }

    if (type == RD) {
        for(int i = 0; i < exe->rd.size(); ++i) {
            if ((exe->rd[i].low <= address) && (address <= exe->rd[i].high)) {
                return true;
            }
        }

        return false;
    }
}

Emulator::~Emulator() {
    if (this->memory != nullptr) {
        delete[] this->memory;
        this->memory = nullptr;
    }
    if (this->exe != nullptr) {

        exe->ex.clear();
        exe->rd.clear();
        exe->rw.clear();
        delete exe;
        exe = nullptr;
    }

    this->interruptBuffer.empty();
    //delete this->timer;
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