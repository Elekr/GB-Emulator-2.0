#pragma once

#include <string>
#include <fstream>

#include "MBC.h"

typedef unsigned __int8 ui8; //8-bit Integer
typedef signed __int8 i8;
typedef unsigned __int16 ui16; //16-bit Integer
typedef signed __int16 i16;


enum CartType
{
    ROM_ONLY = 0x00,
    ROM_AND_MBC1 = 0x01,
    ROM_AND_MBC1_AND_RAM = 0x02,
    ROM_AND_MBC1_AND_RAM_AND_BATT = 0x03
};

//Locations for cart information
const int TITLE_LOC = 0x0134;
const int DEST_CODE = 0x014A;
const int CGB = 0x0143;
const int CART_TYPE = 0x0147;
const int ROM_SIZE = 0x0148;
const int RAM_SIZE = 0x0149;

class Cartridge
{
public:
    ui8* dynamicMemory = nullptr;
    ui8* ram = nullptr;
    ui8* m_bus;

    const char* path;

    std::string gameTitle;

    bool cb;

    int destinationCode;

    int romSize;
    int ramSize;
    ui8 rawRamSize;

    CartType cartType;

    MBC* m_memory_rule;

    Cartridge(ui8* bus);

    bool Load(const char* path);
    void LoadMemoryRule();
    ui8* GetRawData();
    int GetRamSize() { return ramSize; }
    ui8* GetRawRomMemory() { return dynamicMemory; }
    ui8* GetRawRamMemory() { return ram; }
    MBC* GetMBCRule() { return m_memory_rule; }

    ui8 RamBankCount();

};

