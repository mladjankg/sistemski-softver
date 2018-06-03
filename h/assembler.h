#ifndef _SS_ASSEMBLER_H_
#define _SS_ASSEMBLER_H_
#include <string>
#include <fstream>
#include <exception>
#include <unordered_map>
#include "symbol.h"
#include "ss_exceptions.h"
namespace ss {
   
    using SymbolTable = std::unordered_map<std::string, Symbol*>;

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

        std::ifstream *input;

        std::ofstream *output;

        std::unordered_map<std::string, Symbol*> symbolTable;
    };
}

#endif