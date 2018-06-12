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
    
    return new Assembler(in, out, outPretty, startAddress);
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