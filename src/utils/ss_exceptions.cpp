#include "ss_exceptions.h"
using namespace ss;


AssemblingException::AssemblingException(std::string line, const int lineNumber) {
    this->message = "Error at line \n";
    this->message += lineNumber;
    this->message += "\t " + line;
}

const char* AssemblingException::what() const throw () {
    return this->message.c_str();
}

AssemblingException::AssemblingException(std::string msg, std::string line, const int lineNumber) {
    this->message = "\nERROR: " + msg + " at line:\n";
    this->message += std::to_string(lineNumber) + " " + line + "\n";
}

AssemblingException::AssemblingException(const AssemblingException& ae) {
    this->message = ae.message;
}