#ifndef _SS_UTILS_H_
#define _SS_UTILS_H_
#include <string>
#include <utils.h>

using namespace std;

namespace ss {
    
    class Utils {
    public:
        static std::string& trim(std::string& str);
    
        static std::string& removeRepeatingChars(std::string& str, const std::string& chars = Utils::emptyChars);

        static const std::string emptyChars;

        static const std::string empty;
    };
}
#endif