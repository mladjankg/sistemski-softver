#ifndef _SS_ASSEMBLER_H_
#define _SS_ASSEMBLER_H_
#include <regex>
#include <string>
#include <fstream>
#include <exception>
#include <map>
#include <list>
#include <vector>

#include "ss_exceptions.h"
#include "asm_declarations.h"

#define CONDITION_FLAGS_OFFSET 14
#define INSTRUCTION_FLAGS_OFFSET 10
#define OP1_ADDRESSING_FLAGS_OFFSET 8
#define OP2_ADDRESSING_FLAGS_OFFSET 3

#define SWAP_BYTES(x) (((x << 8) & 0xFF00) | ((x >> 8) & 0xFF))

namespace ss {
    
    class Symbol;
    using SymbolTable = std::map<std::string, Symbol*>;
    class Instruction;
    
    class Section;
    class Directive;
    class BWLDirective;
    class Operand;
    class Relocation;

    class Assembler {
    public:
        //Assembler(Assembler&& a);

        void assemble();

        static Assembler* getInstance(std::string& inputFile, std::string& outputFile, unsigned short startAddress);

        ~Assembler();
    private:

        //Private constructor for controlled creation of assembler.
        Assembler(std::ifstream* in, std::ofstream* out, std::ofstream* outPretty, unsigned short startAddress);

        //Method that does the first pass.
        void firstPass();

        //Method that does the second pass.
        void secondPass();

        void writeOutput();

        void writePrettyOutput();

        void changeSection(const std::string& sectionName, SectionType sectionType, Access access, int locationCounter, Section*& previousSection, Section*& currentSection);
        
        //Method that parse directive from line.
        void parseDirective(const std::string& line, const std::string& directive, const int lineNumber, int& locationCounter, Section* currentSection);
        

        //method that gets directive parameters
        std::string getParameters(const std::string line);
        
        //Method that gets directive.
        std::string getDirective(const std::string line) const;
        
        static void initStatic();

        static bool checkReserved(std::string label);

        short resolveLabel(const size_t& locationCounter, Section* current, const std::string label, const int lineNumber, const bool pcRel = false);      
        short resolveDataLabel(const size_t& locationCounter, Section* current, const std::string label, BWLDirective* bwl, const int lineNumber);
        
        void assembleTextSection(Section* current, size_t& locationCounter);       
        void assembleDataSection(Section* current, size_t& locationCounter);      
        void assembleRODataSection(Section* current, size_t& locationCounter);

        void cleanLocalSymbols();
        void copy(const Assembler&);
        void move(Assembler&);

        bool getImmediateValue(const std::string strVal, short& immed);

        char getOperandCode(Operand* op, Section* current, Instruction* i, const size_t& locationCounter, short& secondHalf, const int lineNumber);

        std::ifstream *input;
        std::ofstream *output;
        std::ofstream *objdumpOut;

        short startAddress;
        
        bool canAlign = true;

        std::map<std::string, Symbol*> symbolTable;
        
        std::map<int, std::string> lines;        
        std::map<int, Instruction*> instructions;        
        std::map<int, Directive*> data;
        std::map<int, Directive*> roData;
        
        std::vector<char> textBin;
        std::vector<char> dataBin;
        std::vector<char> roDataBin;

        std::vector<Relocation> relText;
        std::vector<Relocation> relData;
        std::vector<Relocation> relROData;

        std::list<std::string> txtRelData;
        std::list<std::string> txtRelText;
        std::list<std::string> txtRelROData;

        std::string textOut;
        std::string dataOut;
        std::string roDataOut;

        static std::map<std::string, char> reservedWords;

        SectionType sectionOrder[4] = {SectionType::UDF, SectionType::UDF, SectionType::UDF, SectionType::UDF};
        char sectionCounter = 0;
        //std::regex labelRegex;
    };
}

#endif