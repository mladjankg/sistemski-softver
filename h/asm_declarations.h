#ifndef _SS_ASM_DECLARATIONS_
#define _SS_ASM_DECLARATIONS_
//Instructions op codes.
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
    RET
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

#endif