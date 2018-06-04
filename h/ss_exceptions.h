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
        AssemblingException(std::string line, int lineNumber);

        const char* what() const throw();

        AssemblingException(std::string msg, std::string line, int lineNumber = 0);
        
        AssemblingException(const AssemblingException& ae);
        
    private:
        std::string message;
        
        static std::string generigMsg;
    };
}
#endif