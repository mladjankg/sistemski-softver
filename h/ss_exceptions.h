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
        AssemblingException(std::string msg) : message(msg) {}
        
        AssemblingException(std::string line, const int lineNumber);

        const char* what() const throw();

        AssemblingException(std::string msg, std::string line, const int lineNumber = 0);
        
        AssemblingException(const AssemblingException& ae);
        
    private:
        std::string message;
        
        static std::string generigMsg;
    };

    class LinkingException: public std::exception {
    public:
        LinkingException(std::string msg);
        LinkingException(const LinkingException&);
        const char* what() const throw() {
            
            return message.c_str();
        }
    private:
        std::string message;
    };


    class EmulatingException: public std::exception {
    public:
        EmulatingException(std::string msg);
        EmulatingException(const EmulatingException&);
        const char* what() const throw() {
            return message.c_str();
        }
    private:
        std::string message;
    };
}
#endif