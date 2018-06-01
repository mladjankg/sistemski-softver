#ifndef _SS_ASSEMBLER_H_
#define _SS_ASSEMBLER_H_
#include <string>
#include <fstream>
#include <exception>

namespace ss {
    class FileException: public std::exception {
    public:
        FileException(const char* lineContent) :message(lineContent) {}

        const char* what() const throw() override {
            return message;
        }

    private:
        const char* message;
    };

    class AssemblingException: public std::exception {
    public:
        AssemblingException(const char* message, int lineNumber) : message(message), lineNumber(lineNumber) {}


        const char* what() const throw() override {
            std::string ret = "Error at line " + lineNumber;
            ret += lineNumber;
            ret += "\n";
            ret += message;
            return ret.c_str();
        }

    private:
        const char* message;
        int lineNumber;
    };

    class Assembler {
    public:
        Assembler(Assembler&& a);

        void assemble() throw();

        static Assembler getInstance(std::string& inputFile, std::string& outputFile) throw();

        ~Assembler();
    private:

        //Private constructor for controlled creation of assembler.
        Assembler(std::ifstream* in, std::ofstream* out);

        //Method that does the first pass.
        void firstPass() throw();

        //Method that does the second pass.
        void secondPass();

        void copy(const Assembler&);

        void move(Assembler&);

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

        std::ifstream *input;

        std::ofstream *output;
    };
}

#endif