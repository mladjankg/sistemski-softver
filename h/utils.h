#include <string>

using namespace std;

namespace ss {
    
    class Utils {
    public:
        static std::string& trim(std::string& str);
    
        static std::string& removeDuplicateEmptyChars(std::string& str);

    private:
        static const std::string trimChars;
    };
}