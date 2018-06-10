#ifndef _SS_ASM_DECLARATIONS_
#define _SS_ASM_DECLARATIONS_

#define MAX_SHORT 0xFFFF
#define LIMIT_MASK 0xFFFF0000
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
        JMP,
        ADD_JMP //One implementation of JMP, other is with mov
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

    enum OperandType {
        IMMED_VAL,            // 20 – immediate value 20
        LABEL_VAL,            // &x – value of symbol x
        MEMDIR_VAL,           // x – memory direct addressing
        DECIMAL_LOCATION_VAL, // *20 – location from addres 20
        REGDIR_VAL,           // r3 – register direct
        REGIND_DEC_VAL,       // r4[32] – register indirect with immediate offset
        REGIND_LAB_VAL,       // r5[x] – register indirect with variable offset
        PCREL_VAL             // $x – PC relative addresing of variable x
    };
}
#endif