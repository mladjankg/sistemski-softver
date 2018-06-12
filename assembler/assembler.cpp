#include <string>
#include <sstream>
#include <fstream>
#include <map>
#include <list>
#include <iterator>
#include <iostream>
#include <iomanip>
#include <stdexcept>


#include "assembler.h"
#include "utils.h"
#include "string_tokenizer.h"
#include "symbol.h"
#include "ss_exceptions.h"
#include "instruction.h"
#include "operand.h"
#include "section.h"
#include "directive.h"
#include "asm_declarations.h"
#include "relocation.h"
#include "elf.h"

using namespace ss;

std::map<std::string, char> Assembler::reservedWords;

Assembler::Assembler(std::ifstream* in, std::ofstream* out, std::ofstream* outPretty, unsigned short start) : input(in), output(out), objdumpOut(outPretty), startAddress(start) {

}

// Assembler::Assembler(Assembler&& a) {
//     move(a);
// }

void Assembler::move(Assembler& a) {
    this->input = a.input;
    this->output = a.output;
    this->symbolTable = a.symbolTable;
}

Assembler* Assembler::getInstance(std::string& inputFile, std::string& outputFile, unsigned short startAddress) {
    
    //Creating input stream.
    std::ifstream* in = new std::ifstream(inputFile, std::ifstream::in);

    //Creating output stream.
    std::ofstream* out = new std::ofstream(outputFile + ".o", std::ofstream::out | std::ofstream::trunc | std::ofstream::binary);
    
    std::ofstream* outPretty = new std::ofstream(outputFile, std::ofstream::out | std::ofstream::trunc);

    //Checking if output stream was created correctly.
    std::string message = Utils::empty;
    if (!out->is_open()) {
        message += "Cannot open file " + outputFile + ".o" + ".\n";

        throw FileException(message.c_str());
    }


    if (!out->is_open()) {
        message += "Cannot open file " + outputFile + ".\n";

        throw FileException(message.c_str());
    }

    //Checking if input stream was created correctly.
    if (!in->is_open()) {
        message += "Cannot open file " + inputFile + ".\n";
        
        out->close();
        delete out;
  
        throw FileException(message.c_str());
    }
    
    Assembler::initStatic();

    return new Assembler(in, out, outPretty, startAddress);
}

bool Assembler::checkReserved(std::string label) {
    if (Assembler::reservedWords.find(label) != Assembler::reservedWords.end()) {
        return true;
    }

    if ((label.length() > 3) && (label.length() < 7)) {
        std::string lab = label.substr(0, label.length() - 2);
        std::string sufix = label.substr(label.length() - 2);
        bool addrSufix = !sufix.compare("eq") || !sufix.compare("al") || !sufix.compare("gt") || !sufix.compare("ne");

        if (Assembler::reservedWords.find(lab) != Assembler::reservedWords.end()) {
            return Assembler::reservedWords[lab] && addrSufix;
        }
    }

    return false;
}
void Assembler::initStatic() {
    Assembler::reservedWords["r0"] = 0;
    Assembler::reservedWords["r1"] = 0;
    Assembler::reservedWords["r2"] = 0;
    Assembler::reservedWords["r3"] = 0;
    Assembler::reservedWords["r4"] = 0;
    Assembler::reservedWords["r5"] = 0;
    Assembler::reservedWords["r6"] = 0;
    Assembler::reservedWords["r7"] = 0;
    Assembler::reservedWords["pc"] = 0;
    Assembler::reservedWords["sp"] = 0;
    Assembler::reservedWords["psw"] = 0;
    Assembler::reservedWords["add"] = 1;
    Assembler::reservedWords["sub"] = 1;
    Assembler::reservedWords["mul"] = 1;
    Assembler::reservedWords["div"] = 1;
    Assembler::reservedWords["cmp"] = 1;
    Assembler::reservedWords["and"] = 1;
    Assembler::reservedWords["or"] = 1;
    Assembler::reservedWords["not"] = 1;
    Assembler::reservedWords["test"] = 1;
    Assembler::reservedWords["push"] = 1;
    Assembler::reservedWords["pop"] = 1;
    Assembler::reservedWords["call"] = 1;
    Assembler::reservedWords["iret"] = 1;
    Assembler::reservedWords["mov"] = 1;
    Assembler::reservedWords["shl"] = 1;
    Assembler::reservedWords["shr"] = 1;
}

void Assembler::assemble() {
    this->firstPass();
    this->secondPass();
    this->cleanLocalSymbols();
    this->writeOutput();
    this->writePrettyOutput();
}

void Assembler::cleanLocalSymbols() {
    std::vector<std::string> toDelete;

    for(auto it = this->symbolTable.begin(); it != this->symbolTable.end(); ++it) {
        if (it->second->isLocal() && (it->second->getSectionPtr() != nullptr))
            toDelete.push_back(it->second->getName());
    }

    for(int i = 0; i < toDelete.size(); ++i) {
        this->symbolTable.erase(toDelete[i]);
    }
}

Assembler::~Assembler() {

    //Closing streams.
    this->input->close();
    this->output->close();
    this->objdumpOut->close();
    //Deleting input stream.
    delete this->input;
    this->input = nullptr;

    //Deleting output stream.
    delete this->output;
    this->output = nullptr;
    
    delete this->objdumpOut;
    this->objdumpOut = nullptr;

    for(auto it = this->symbolTable.begin(); it != this->symbolTable.end(); it++) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        } 
    }
        
    for(auto it = this->instructions.begin(); it != this->instructions.end(); it++) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        } 
    }

    for(auto it = this->data.begin(); it != this->data.end(); it++) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        } 
    } 
    
    for(auto it = this->roData.begin(); it != this->roData.end(); it++) {
        if (it->second != nullptr) {
            delete it->second;
            it->second = nullptr;
        } 
    } 
}