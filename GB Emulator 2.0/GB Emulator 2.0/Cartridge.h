#pragma once

#include <string>
#include <fstream>

#include "MBC.h"

typedef unsigned __int8 ui8; //8-bit Integer
typedef signed __int8 i8;
typedef unsigned __int16 ui16; //16-bit Integer
typedef signed __int16 i16;


enum CartType //All cartridge Types (for later cartridge implementation)
{
	ROM_ONLY = 0x00,
	ROM_AND_MBC1 = 0x01,
	ROM_AND_MBC1_AND_RAM = 0x02,
	ROM_AND_MBC1_AND_RAM_AND_BATT = 0x03,
	ROM_AND_MBC2 = 0x05,
	ROM_AND_MBC2_AND_BATT = 0x06,
	ROM_AND_RAM = 0x08,
	ROM_AND_RAM_AND_BATT = 0x09,
	ROM_AND_MMMD1 = 0x0B,
	ROM_AND_MMMD1_AND_SRAM = 0x0C,
	ROM_AND_MMMD1_AND_SRAM_AND_BATT = 0x0D,
	ROM_AND_MBC3_AND_TIMER_AND_BATT = 0x0F,
	ROM_ANDMMMD1_AND_SRAM_AND_BATT = 0x10,
	ROM_AND_MBC3 = 0x11,
	ROM_AND_MBC3_AND_RAM = 0x12,
	ROM_AND_MBC3_AND_RAM_BATT = 0x13,
	ROM_AND_MBC5 = 0x19,
	ROM_AND_MBC5_AND_RAM = 0x1A,
	ROM_AND_MBC5_AND_RAM_AND_BATT = 0x1B,
	ROM_AND_MBC5_AND_RUMBLE = 0x1C,
	ROM_AND_MBC5_AND_RUMBLE_AND_SRAM = 0x1D,
	ROM_AND_MBC5_AND_RUMBLE_AND_SRAM_AND_BATT = 0x1E,
	POCKET_CAMERA = 0x1F,
	BANDAI_TAMA5 = 0xFD,
	HUDSON_HUC_3 = 0xFE,
	HUDSON_HUC_1 = 0xFF
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
    ui8* ROMData = nullptr;
    ui8* RAMData = nullptr;
    ui8* m_bus;

    const char* path;

    std::string gameTitle;

    bool cb;

    int destinationCode;

    int romSize;
    int ramSize;
    ui8 rawRamSize;

    CartType cartType;

    MBC* memoryBankController;

    Cartridge(ui8* bus);

    bool LoadCartridge(const char* path);
    void LoadMBC();

    ui8* GetROMData() { return ROMData; }
    ui8* GetRAMData() { return RAMData; }
	int GetRamSize() { return ramSize; }
    MBC* GetMBCType() { return memoryBankController; }

    ui8 RamBankCount();

};

