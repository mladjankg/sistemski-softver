#include <iostream>
#include <string>
#include <regex>
#include "string_tokenizer.h"
#include "utils.h"
#include "assembler.h"

using namespace ss;

const std::string usage = "assembler <input> [<output>]";


int main(int argc, const char* argv[]) {
    if (argc < 3) {
        std::cout << "ERROR: insufficient number of parameters.\n" << usage << std::endl;
        return -1;
    }

    if (argc > 4) {
        std::cout << "ERROR: too many parameters.\n" << usage << std::endl;
        return -1;
    }

    std::string input(argv[1]);
    std::string output(argv[2]);

    int start;
    if (argc == 3) start = 0;
    else start = std::stoi(argv[3]);
    
    //Creating assembler.
    try {
        Assembler* as = Assembler::getInstance(input, output, start);

        as->assemble();

        delete as;
    }
    catch (FileException e) {
        std::cout << e.what();
    }
    catch (AssemblingException& e) {
        std::cout << e.what();
    }
    catch (StringTokenizerException e) {
        
    }
    catch (std::exception e) {
        std::cout<<e.what();
    }
    std::cout<<"\n==========END==========\n" << std::flush;

    return 0;
}


