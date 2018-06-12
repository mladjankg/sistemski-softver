#include <iomanip>
#include <sstream>
#include "relocation.h"

using namespace ss;

std::string Relocation::toString() const {
    std::string relType = this->type == RelocationType::R_386_PC16 ? "R_386_PC16" : this->type == RelocationType::R_386_16 ? "R_386_16" : "R_386_32";

    std::stringstream stream;
    stream << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << std::hex << this->offset
           << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << relType
           << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << this->id;

    return stream.str();
}