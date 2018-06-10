#ifndef _SS_UTILS_H_
#define _SS_UTILS_H_
#include <string>
#include <regex>

namespace ss {
    
    class Utils {
    public:
        static std::string& trim(std::string& str);
    
        static std::string trim(const std::string& str) {
            std::string s(str);
            return trim(s);
        }
        
        static const std::string emptyChars;
                
        static std::string& removeRepeatingChars(std::string& str, const std::string& chars = emptyChars);

        static std::string removeEmptySpaces(const std::string& str);
        
        static const std::string empty;

        static std::regex labelRegex, decimalRegex;
    };
}
#endif