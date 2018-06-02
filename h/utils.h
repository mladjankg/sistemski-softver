#ifndef _SS_UTILS_H_
#define _SS_UTILS_H_
#include <string>

using namespace std;

namespace ss {
    
    class Utils {
    public:
        static std::string& trim(std::string& str);
    
        static const std::string emptyChars;
                
        static std::string& removeRepeatingChars(std::string& str, const std::string& chars = emptyChars);

        static const std::string empty;
    };
}
#endif