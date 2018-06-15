#include "linker.h"
#include "ss_exceptions.h"
#include "emulator.h"
#include <iostream>
using namespace ss;

int main(int argc,  const char* argv[]) {
    Linker linker;

    const char** args = &argv[1];

    try {
        auto exe = linker.linkFiles(args, argc - 1);
        Emulator emulator(exe);
        emulator.startEmulation();
    }
    catch(LinkingException e) {
        std::cout << e.what() << std::flush;
    }
    catch(std::exception& e) {
        std::cout << e.what() << std::flush;
    }

    return 0;
}