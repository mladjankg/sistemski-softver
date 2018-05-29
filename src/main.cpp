#include <iostream>
#include <string>
#include "string_tokenizer.h"
#include "utils.h"
#include <regex>

using namespace ss;

void tokenizerTest() {
    const std::string line = "mov r12, r4";
    const std::regex e ("[ \t\r\n\f]*([a-zA-Z0-9_]+)[ \t\r\n\f,]*f");
   
    std::smatch match;

    std::regex_search(line, match, e);

    for (auto x:match)
        std::cout << x << " ";
}


void testTrim() {
    std::string line = "         mov r12, r4   \n";

    line = Utils::trim(line);

    std::cout << line + "\n" << std::endl;
}

void testRemoveEmptyEntries() {
    std::string line = "    kpkpk joj      joj j\t   ppkpk \v\f    refwef  \n\n";
    line = Utils::removeDuplicateEmptyChars(line);
    std::cout << line << endl;
}


int main() {
    testRemoveEmptyEntries();
}


