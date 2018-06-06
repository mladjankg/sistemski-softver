#ifndef _SS_ASSEMBLER_H_
#define _SS_ASSEMBLER_H_
#include <string>
#include <fstream>
#include <exception>
#include <unordered_map>
#include "symbol.h"
#include "ss_exceptions.h"
#include "asm_declarations.h"

namespace ss {
   
    using SymbolTable = std::unordered_map<std::string, Symbol*>;
    class Instruction;
    class Section;
    
    class Assembler {
    public:
        Assembler(Assembler&& a);

        void assemble();

        static Assembler getInstance(std::string& inputFile, std::string& outputFile);

        ~Assembler();
    private:

        //Private constructor for controlled creation of assembler.
        Assembler(std::ifstream* in, std::ofstream* out);

        //Method that does the first pass.
        void firstPass();

        //Method that does the second pass.
        void secondPass();

        void changeSection(const std::string& sectionName, SectionType sectionType, Access access, int locationCounter, Section*& previousSection, Section*& currentSection);
        
        void parseDirective(const std::string& line, const int lineNumber, int& locationCounter, Section* currentSection);
        
        std::string getParameters(const std::string line);
        
        void copy(const Assembler&);

        void move(Assembler&);

        std::ifstream *input;

        std::ofstream *output;

        std::unordered_map<std::string, Symbol*> symbolTable;
        
        std::unordered_map<int, std::string> lines;
        
        std::unordered_map<int, Instruction*> instructions;
        
        std::regex labelRegex;
    };
}

#endif