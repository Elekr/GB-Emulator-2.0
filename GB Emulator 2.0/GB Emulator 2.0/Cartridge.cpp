#include "Cartridge.h"
#include <string>
#include <iostream>

bool Cartridge::Load(const char* path)
{
	//http://www.cplusplus.com/doc/tutorial/files/
	ifstream file(path, ios::ate | std::ios::binary); //ate sets the initial position to the end of the file (to parse the file size)

	if (!file.is_open())
	{
		return false;
	}

	//https://stackoverflow.com/questions/2409504/using-c-filestreams-fstream-how-can-you-determine-the-size-of-a-file
	unsigned int fileSize = static_cast<int>(file.tellg());

	mp_cart_data = new ui8[fileSize]; //Allocate memory for the rom 
	file.seekg(0, std::ios::beg);
	file.read((char*)mp_cart_data, fileSize); //Cast to char
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
		name[i] = mp_cart_data[TITLE_LOC + i];	
	}
	gameTitle = name;

	mp_cart_data[CGB] == 0x80;

	cartType = (CartType)mp_cart_data[CART_TYPE];

	destinationCode = mp_cart_data[DEST_CODE];

	ramSize = mp_cart_data[RAM_SIZE];

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
