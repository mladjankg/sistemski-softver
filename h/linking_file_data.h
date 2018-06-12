#ifndef _SS_LINKING_FILE_DATA_
#define _SS_LINKING_FILE_DATA_

#include "elf.h"

namespace ss {
    struct LinkingFileData {
        SectionHeader* secHeaders;
        ELFHeader header;

        ~LinkingFileData() {
            delete[] secHeaders;
        }
    };
}
#endif