#ifndef _SS_ASSEMBLER_H_
#define _SS_ASSEMBLER_H_
#include <regex>
#include <string>
#include <fstream>
#include <exception>
#include <map>
#include <list>
#include "symbol.h"
#include "ss_exceptions.h"
#include "asm_declarations.h"

#define SECTION_NUMBER 4
#define CONDITION_FLAGS_OFFSET 14
#define INSTRUCTION_FLAGS_OFFSET 10
#define ADDRESSING_FLAGS_OFFSET 8

namespace ss {
    

    using SymbolTable = std::map<std::string, Symbol*>;
    class Instruction;
    class Section;
    class Directive;
    
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
        
        //Method that parse directive from line.
        void parseDirective(const std::string& line, const std::string& directive, const int lineNumber, int& locationCounter, Section* currentSection);
        
        //method that gets directive parameters
        std::string getParameters(const std::string line);
        
        //Method that gets directive.
        std::string getDirective(const std::string line) const;
        
        size_t resolveLabel(const size_t& locationCounter, Section* current, const std::string label);

        void assembleTextSection(Section* current, size_t& locationCounter);
        void assembleDataSection(Section* current, size_t& locationCounter);
        void assembleRODataSection(Section* current, size_t& locationCounter);

        void copy(const Assembler&);

        void move(Assembler&);

        std::ifstream *input;

        std::ofstream *output;

        std::map<std::string, Symbol*> symbolTable;
        
        std::map<int, std::string> lines;
        
        std::map<int, Instruction*> instructions;
        
        std::map<int, Directive*> data;

        std::map<int, Directive*> roData;
        
        std::list<std::string> relData;

        std::list<std::string> relText;

        SectionType sectionOrder[4] = {SectionType::UDF, SectionType::UDF, SectionType::UDF, SectionType::UDF};
        //std::regex labelRegex;
    };
}

#endif