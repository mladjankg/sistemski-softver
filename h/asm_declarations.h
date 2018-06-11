#ifndef _SS_ASM_DECLARATIONS_
#define _SS_ASM_DECLARATIONS_

#define MAX_SHORT 0xFFFF
#define MAX_BYTE_MASK 0xFFFFFF00
#define LIMIT_MASK 0xFFFF0000
#define FIELD_LENGTH 15
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
        STR_TAB,
        UDF
    };

    enum DirectiveType: char { //DO NOT CHANGE THE ORDER, IS USED FOR DETERMING DIRECTIVE SPACE IN MEMORY
        BYTE,
        WORD,
        LONG,
        SKIP,
        ALIGN
    };

    enum OperandType : char {
        IMMED_VAL,            // 20 – immediate value 20
        LABEL_VAL,            // &x – value of symbol x
        MEMDIR_VAL,           // x – memory direct addressing
        DECIMAL_LOCATION_VAL, // *20 – location from addres 20
        REGDIR_VAL,           // r3 – register direct
        REGIND_DEC_VAL,       // r4[32] – register indirect with immediate offset
        REGIND_LAB_VAL,       // r5[x] – register indirect with variable offset
        PCREL_VAL             // $x – PC relative addresing of variable x
    };

    enum RelocationType : char {
        R_386_PC16,
        R_386_16,
        R_386_32
    };
}
#endif