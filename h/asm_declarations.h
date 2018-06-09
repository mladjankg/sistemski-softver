#ifndef _SS_ASM_DECLARATIONS_
#define _SS_ASM_DECLARATIONS_

#define MAX_SHORT 0xFFFF
//Instructions op codes.
namespace ss {
    
    enum InstructionCode : char {
        ADD,
        SUB,
        MUL,
        DIV,
        CMP,
        AND,
        OR,
        NOT,
        TEST,
        PUSH,
        POP,
        CALL,
        IRET,
        MOV,
        SHL,
        SHR,
        RET,
        JMP
    };

    //Addressing codes
    enum AddressingCode : char {
        IMMED,
        REGDIR,
        MEMDIR,
        REGINDPOM
    };

    //Condition codes
    enum ConditionCode : char {
        EQ, // ==
        NE, // !=
        GT, // >
        AL  // unconditional
    };

    enum Access: char {
        RD,
        WR,
        RW,
        EX   
    };

    enum SectionType: char {
        TEXT,
        DATA,
        RO_DATA,
        BSS,
        UDF
    };

    enum DirectiveType: char {
        BYTE,
        WORD,
        LONG,
        SKIP
    };
}
#endif