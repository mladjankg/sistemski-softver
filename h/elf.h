#ifndef _SS_ELF_H_
#define _SS_ELF_H_

#include "asm_declarations.h"

namespace ss {
    typedef unsigned short ElfWord;

    struct ELFHeader {
        ElfWord entry;        //Ulazna adresa programa ako je definisana
        //short phOff;        //Offset do tabele programa (ovo mozda nece trebati)
        ElfWord shOff;        //Offset do tabele zaglavlja sekcija
        ElfWord ehSize;       //Velicina elf headera
        ElfWord shEntSize;   //Velicina jednog ulaza u zaglavlju sekcija
        ElfWord shNum;        //Broj ulaza u tabeli zaglavlja sekcija
    };

    struct SectionHeader {
        SectionHeader() {}
        SectionHeader(SectionType type, Access access, ElfWord offset, ElfWord size, ElfWord addrAlign, size_t entSize)
            : type(type), access(access), offset(offset), size(size), addrAlign(addrAlign), entSize(entSize) {}

        SectionHeader& operator= (SectionHeader&& sh) {
            type = sh.type;
            access = sh.access;
            offset = sh.offset;
            size = sh.size;
            addrAlign = sh.addrAlign;
            entSize = sh.entSize;

            return *this;
        }

        SectionType type;   //Tip sekcije
        Access access;      //Prava pristupa
        ElfWord offset;       //Offset prvog bajta sekcije u odnosu na pocetak fajla
        ElfWord size;         //Velicina sekcije
        ElfWord addrAlign;    //Poravnanje sekcije
        size_t entSize;     //Velicina jednog ulaza u tabeli
    };

    struct SymTabEntry {
        SymTabEntry(ElfWord name, ElfWord offset, SectionType section, ElfWord id)
            : name(name), offset(offset), section(section), id(id) {}
        ElfWord name;             //Indeks u tabeli stringova
        ElfWord offset;           //Offset simbola u odnosu na pocetak sekcije
        //ElfWord size;
        SectionType section;    //Sekcija u kojoj se simbol nalazi
        ElfWord id;
    };
}
#endif