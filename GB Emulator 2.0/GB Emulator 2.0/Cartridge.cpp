#include "Cartridge.h"
#include "RomOnly.h"
#include <string>
#include <iostream>

bool Cartridge::Load(const char* path)
{
	//http://www.cplusplus.com/doc/tutorial/files/
	std::ifstream file(path, std::ios::ate | std::ios::binary); //ate sets the initial position to the end of the file (to parse the file size)

	if (!file.is_open())
	{
		return false;
	}

	//https://stackoverflow.com/questions/2409504/using-c-filestreams-fstream-how-can-you-determine-the-size-of-a-file
	unsigned int fileSize = static_cast<int>(file.tellg());

	dynamicMemory = new ui8[fileSize]; //Allocate memory for the rom 
	file.seekg(0, std::ios::beg);
	file.read((char*)dynamicMemory, fileSize); //Cast to char
	file.close();

	//Used for debugging issues with loading the cart in
	//for (int i = 0; i < 0x100; i++)
	//{
	//	cout << hex << i << " " << hex << (int)mp_cart_data[i] << std::endl;
	//}

	//Get the title of the Game
	char* name = new char[11];

	for (int i = 0; i < 10; i++)
	{
		name[i] = dynamicMemory[TITLE_LOC + i];	
	}
	gameTitle = name;

	dynamicMemory[CGB] == 0x80;

	cartType = (CartType)dynamicMemory[CART_TYPE];

	destinationCode = dynamicMemory[DEST_CODE];

	ramSize = dynamicMemory[RAM_SIZE];

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
	if (rawRamSize > 0) //If there's ram 
	{
		ram = new ui8[rawRamSize];
	}

	return true;
}

void Cartridge::LoadMemoryRule()
{
	switch (cartType)
	{
	case CartType::ROM_ONLY:
	{
		m_memory_rule = new RomOnly(this, m_bus);
	}
	break;
	/*case GBCartridgeType::ROM_AND_MBC1:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC1, CartRam::None, CartBatt::None>>(this, m_bus);
	}
	break;

	case GBCartridgeType::ROM_AND_MBC1_AND_RAM:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC1, CartRam::Avaliable, CartBatt::None>>(this, m_bus);
	}
	break;


	case GBCartridgeType::ROM_AND_MBC1_AND_RAM_AND_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC1, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;

	case GBCartridgeType::ROM_ANDMMMD1_AND_SRAM_AND_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC3, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;



	case GBCartridgeType::ROM_AND_MBC3_AND_RAM_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC3, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;

	case GBCartridgeType::ROM_AND_MBC5_AND_RAM_AND_BATT:
	{
		m_memory_rule = std::make_unique<MBCN<CartMBC::MBC5, CartRam::Avaliable, CartBatt::Avaliable>>(this, m_bus);
	}
	break;*/

	default:
		exit(0);
		break;
	}
}

ui8* Cartridge::GetRawData()
{
	return dynamicMemory;
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
