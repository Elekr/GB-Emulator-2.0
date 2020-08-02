#include "Cartridge.h"
#include "RomOnly.h"
#include "MBC1.h"
#include <string>
#include <iostream>

Cartridge::Cartridge(ui8* bus) : m_bus(bus)
{

}

bool Cartridge::LoadCartridge(const char* path)
{
	//http://www.cplusplus.com/doc/tutorial/files/
	std::ifstream file(path, std::ios::ate | std::ios::binary); //ate sets the initial position to the end of the file (to parse the file size)

	if (!file.is_open())
	{
		return false;
	}

	//https://stackoverflow.com/questions/2409504/using-c-filestreams-fstream-how-can-you-determine-the-size-of-a-file
	unsigned int fileSize = static_cast<int>(file.tellg());

	ROMData = new ui8[fileSize]; //Allocate memory for the rom 
	file.seekg(0, std::ios::beg);
	file.read((char*)ROMData, fileSize); //Cast to char
	file.close();

	//Used for debugging issues with loading the cart in
	//for (int i = 0; i < 0x100; i++)
	//{
	//	cout << hex << i << " " << hex << (int)mp_cart_data[i] << std::endl;
	//}

	//Get the title of the Game

	char* name = new char[20];

	for (int i = 0; i < 20; i++)
	{
		name[i] = ROMData[TITLE_LOC + i];	
	}
	gameTitle = name;

	ROMData[CGB] == 0x80; //Set to not color mode (Gameboy Color not implemented yet)

	cartType = (CartType)ROMData[CART_TYPE];

	destinationCode = ROMData[DEST_CODE];
	romSize = ROMData[ROM_SIZE];
	ramSize = ROMData[RAM_SIZE];

	std::cout << "Game: " << gameTitle << std::endl;
	std::cout << "Cartridge Type: " << cartType << std::endl;
	std::cout << "ROM Banks: " << romSize << std::endl;
	std::cout << "RAM Size: " << ramSize << std::endl;
	std::cout << "Destination Code: " << destinationCode << std::endl;

	switch (ramSize)
	{
	case 1:
		rawRamSize = 0x1024 * 2;
		break;
	case 2:
		rawRamSize = 0x1024 * 8;
		break;
	case 3:
		rawRamSize = 0x1024 * 32;
		break;
	case 4:
		rawRamSize = 0x1024 * 128;
		break;
	}

	LoadMBC();

	if (rawRamSize > 0)
	{
		RAMData = new ui8[rawRamSize]{ 0 }; //Create RAM if it exists
	}

	return true;
}

void Cartridge::LoadMBC()
{
	switch (cartType)
	{
	case CartType::ROM_ONLY:
	{
		memoryBankController = new RomOnly(this, m_bus);
	}
	break;
	case CartType::ROM_AND_MBC1:
	{
		memoryBankController = new MBC1(this, m_bus);
	}
	break;
	case CartType::ROM_AND_MBC1_AND_RAM:
	{
		memoryBankController = new MBC1(this, m_bus);
	}
	break;
	case CartType::ROM_AND_MBC1_AND_RAM_AND_BATT:
	{
		memoryBankController = new MBC1(this, m_bus);
	}
	break;
	case CartType::ROM_AND_MBC3_AND_RAM_BATT:
	{
		memoryBankController = new MBC1(this, m_bus);
	}
	break;
	default:
		exit(0); //Unknown Cart type
		break;
	}
}

ui8 Cartridge::RamBankCount()
{
	switch (ramSize)
	{
	case 0:
		return 0;
	case 1:
	case 2:
		return 1;
	case 3:
		return 4;
	case 4:
		return 16;
	}
	return 0;
}
