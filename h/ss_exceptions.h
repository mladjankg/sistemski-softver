#ifndef _SS_EXCEPITONS_H_
#define _SS_EXCEPITONS_H_

#include <string>
#include <exception>

namespace ss {
    class StringTokenizerException: public std::exception {
        
    };

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
}
#endif