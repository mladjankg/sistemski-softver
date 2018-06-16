#include "linker.h"
#include "ss_exceptions.h"
#include "emulator.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace ss;


int main(int argc,  const char* argv[]) {
    Linker linker;

    const char** args = &argv[1];

    try {
        auto exe = linker.linkFiles(args, argc - 1);
        Emulator emulator(exe);
        emulator.startEmulation();

        std::cout << "\nEMULATOR CLOSED\n";
    }
    // catch(LinkingException& e) {
    //     std::cout << e.what();
    // }
    catch(std::exception& e) {
        std::cout << e.what();
    }

    return 0;
}