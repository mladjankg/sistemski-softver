#include "utils.h"
#include <string>

using namespace ss;

const std::string Utils::trimChars = "\n\t\r\v\f ";

std::string& Utils::trim(std::string& str) {
    size_t front = str.find_first_not_of(Utils::trimChars);

    if (front != string::npos) {
        str = str.substr(front);
    }

    size_t back = str.find_last_not_of(Utils::trimChars);
    if (back != string::npos) {
        str = str.substr(0, back + 1);
    }

    return str;
}

std::string& Utils::removeDuplicateEmptyChars(std::string& str) {
    std::string emptyChars = " \t\n\r\v\f";
    int j = 0;
    bool found = false;
    std::string oldStr = std::string(str);
    for(int i = 0; i < oldStr.length(); i++) {
        char c = oldStr[i];
        if (emptyChars.find(c) != std::string::npos) {
            if (found) {
                continue;
            }
            else {
                str[j++] = ' ';
                found = true;
            }
        }
        else {
            str[j++] = c;
            found = false;
        }
    }

    str = str.substr(0, j);
    return str;
}