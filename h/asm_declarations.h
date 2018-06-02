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
    SHR
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