#include "linker.h"

using namespace ss;

int main(int argc,  const char* argv[]) {
    Linker linker;

    const char** args = &argv[1];
    linker.linkFiles(args, argc - 1);
}