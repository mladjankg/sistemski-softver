#include <string>
#include <fstream>
#include "assembler.h"
#include "utils.h"
#include "string_tokenizer.h"

using namespace ss;

Assembler::Assembler(std::ifstream* in, std::ofstream* out) : input(in), output(out) {
    
}

Assembler::Assembler(Assembler&& a) {
    move(a);
}

void Assembler::move(Assembler& a) {
    this->input = a.input;
    this->output = a.output;
}

Assembler Assembler::getInstance(std::string& inputFile, std::string& outputFile) throw() {
    
    //Creating input stream.
    std::ifstream* in = new std::ifstream(inputFile, std::ifstream::in);

    //Creating output stream.
    std::ofstream* out = new std::ofstream(outputFile, std::ofstream::out | std::ofstream::trunc);
    
    //Checking if output stream was created correctly.
    std::string message = Utils::empty;
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
    
    return Assembler(in, out);
}

void Assembler::assemble() throw() {
    this->firstPass();
}
    
void Assembler::firstPass() throw() {
    std::string line;
    int lineNumber = 0;

    while(std::getline(*(this->input),line)) {
        ++lineNumber;

        //TODO: Ako je dosao kraj fajla a nije bilo .end-a mora da se prijavi greska.
        if (line == "EOF") break;

        //If we read line that equals to .end, we reached end of file.
        line = Utils::trim(line);
        if (".end") break;

        //If line consist only of empty chars, loop is proceeding to the next line.
        if (line.find_last_not_of(Utils::emptyChars) == std::string::npos) {
            continue;
        }

        //Parsing one line
        if (line.find_first_of(':')) {
            StringTokenizer st(":");
            st.tokenize(line);     
            if (st.tokenNumber() > 2 || st.tokenNumber() < 1) {
                throw AssemblingException(line, lineNumber);
            }   
        }

    }
}

void Assembler::secondPass() {

}

Assembler::~Assembler() {

    //Closing streams.
    this->input->close();
    this->output->close();

    //Deleting input stream.
    delete this->input;
    this->input = nullptr;

    //Deleting output stream.
    delete this->output;
    this->output = nullptr;
}