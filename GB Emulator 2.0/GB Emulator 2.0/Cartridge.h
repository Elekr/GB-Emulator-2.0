#pragma once

#include <string>
#include <fstream>

using namespace std;

typedef unsigned __int8 ui8; //8-bit Integer
typedef signed __int8 i8;
typedef unsigned __int16 ui16; //16-bit Integer
typedef signed __int16 i16;


enum CartType
{
    ROM_ONLY = 0x00,
    MBC1 = 0x01,
    MBC1_RAM = 0x02,
    MBC1_RAM_BATTERY = 0x03,
    MBC2 = 0x05,
    MBC2_BATTERY = 0x06,
    ROM_RAM = 0x08,
    ROM_RAM_BATTERY = 0x09,
    MMM01 = 0x0B,
    MMM01_RAM = 0x0C,
    MMM01_RAM_BATTERY = 0x0D,
    MBC3_TIMER_BATTERY = 0x0F,
    MBC3_TIMER_RAM_BATTERY = 0x10,
    MBC3 = 0x11,
    MBC3_RAM = 0x12,
    MBC3_RAM_BATTERY = 0x13,
    MBC5 = 0x19,
    MBC5_RAM = 0x1A,
    MBC5_RAM_BATTERY = 0x1B,
    MBC5_RUMBLE = 0x1C,
    MBC5_RUMBLE_RAM = 0x1D,
    MBC5_RUMBLE_RAM_BATTERY = 0x1E,
    MBC6 = 0x20,
    MBC7_SENSOR_RUMBLE_RAM_BATTERY = 0x22,
    POCKET_CAMERA = 0xFC,
    BANDAI_TAMA5 = 0xFD,
    HuC3 = 0xFE,
    HuC1_RAM_BATTERY = 0xFF
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
    ui8* mp_cart_data = nullptr;
    ui8* ram = nullptr;

    const char* path;
    string gameTitle;
    bool cb;
    int romSize;
    int ramSize;
    int destinationCode;

    CartType cartType;
    ui8 rawRamSize;

    bool Load(const char* path);
    ui8* GetRawData()
    {
        return mp_cart_data;
    }

};

