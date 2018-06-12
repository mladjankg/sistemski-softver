#include "utils.h"
#include <string>

using namespace ss;

const std::string Utils::emptyChars = " \n\t\v\f\r";
const std::string Utils::empty = "";
std::regex Utils::labelRegex("((^[a-zA-Z_]\\w*$)|(^\.bss$)|(^\.text$)|(^\.data$)|(^\.rodata$))");
std::regex Utils::decimalRegex("(^(-)?[0-9]+$)");

std::string& Utils::trim(std::string& str) {
    size_t front = str.find_first_not_of(Utils::emptyChars);

    if (front != std::string::npos) {
        str = str.substr(front);
    }

    size_t back = str.find_last_not_of(Utils::emptyChars);
    if (back != std::string::npos) {
        str = str.substr(0, back + 1);
    }

    return str;
}

std::string& Utils::removeRepeatingChars(std::string& str, const std::string& chars) {
    //std::string emptyChars = " \t\n\r\v\f";
    int j = 0;
    bool found = false;
    std::string oldStr = std::string(str);
    for(int i = 0; i < oldStr.length(); i++) {
        char c = oldStr[i];
        if (chars.find(c) != std::string::npos) {
            if (found) {
                continue;
            }
            else {
                str[j++] = chars[0];
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

std::string Utils::removeEmptySpaces(const std::string& str) {
    std::string newStr(str);

    int j = 0;
    for(int i = 0; i < str.size(); i++) {
        if (Utils::emptyChars.find(str[i]) == std::string::npos) {
            ++j;
            continue;
        }

        newStr[j++] = str[i];
    }

    newStr = newStr.substr(0, j);

    return newStr;
}