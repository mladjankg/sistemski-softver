#include "linker.h"
#include "ss_exceptions.h"
#include "emulator.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace ss;


int main(int argc,  const char* argv[]) {


    const char** args = &argv[1];

    try {
        Linker linker;
        auto exe = linker.linkFiles(args, argc - 1);
        Emulator emulator(exe);
        emulator.startEmulation();
        exe = nullptr;
        std::cout << "\nEMULATOR CLOSED\n" << std::flush;
        
    }
    // catch(LinkingException& e) {
    //     std::cout << e.what();
    // }
    catch(std::exception& e) {
        std::cout << e.what() << std::flush;
    }
    std::cout << std::flush;
    return 0;
}