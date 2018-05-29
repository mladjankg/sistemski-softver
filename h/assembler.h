namespace ss {
    class Assembler {
    public:
        void assemble();
    private:
    
        void firstPass();

        void secondPass();
        
        //Instructions op codes.
        enum Instruction: char {
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
        enum Addressing: char {
            IMMED,
            REGDIR,
            MEMDIR,
            REGINDPOM
        };
    };
}