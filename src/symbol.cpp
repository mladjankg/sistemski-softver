#include "symbol.h"
#include "section.h"
#include <iostream>
#include <sstream>
#include <iomanip>

using namespace ss;

std::string Symbol::toString() const {
    std::stringstream os;

    os << std::left << std::setw(FIELD_LENGTH) << std::setfill(' ') << this->getName() << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH);
    std::string sectionName;
    size_t offset;
    if (this->getSectionPtr() != nullptr) {
        sectionName = this->getSectionPtr()->getName().substr(1);
        offset = this->getOffset() - this->getSectionPtr()->getOffset();
    }
    else {
        offset = 0;
        if (this->getSectionCode() == SectionType::UDF) {
            sectionName = "udf";
            
        }
        else {
            sectionName = this->getName().substr(1);
            offset = this->getOffset();
        }
    }
    os << sectionName << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << offset << std::left << std::setfill(' ') << std::setw(FIELD_LENGTH) << this->getNo();
    
    return os.str();
}

std::ostream& operator<< (std::ostream& os, const Symbol& s) {
    
    os << s.getName() << '\t';
    std::string sectionName;
    size_t offset;
    if (s.getSectionPtr() != nullptr) {
        sectionName = s.getSectionPtr()->getName().substr(1);
        offset = s.getOffset() - s.getSectionPtr()->getOffset();
    }
    else {
        offset = 0;
        if (s.getSectionCode() == SectionType::UDF) {
            sectionName = "udf";
        }
        else {
            sectionName = s.getName();
        }
    }
    os << sectionName << '\t' << offset << '\t' << s.getNo();
    return os;
}