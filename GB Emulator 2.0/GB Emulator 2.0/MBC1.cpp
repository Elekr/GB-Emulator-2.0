#include "MBC1.h"
#include <iostream>
MBC1::MBC1(Cartridge* cart, ui8* bus) : MBC(cart, bus)
{
	currentBank = 1;
	currentMode = 0;
	bankHigh = 0;
	currentRAMBank = 0;
	RAM_Enabled = false;
}

ui8* MBC1::GetRomBank0()
{
	return &cart->GetROMData()[0x0000];
}

ui8* MBC1::GetRomBank1()
{
	return &cart->GetROMData()[0x4000 * currentBank];
}

void MBC1::Write(ui16 address, ui8 data)
{
	switch (address & 0xE000) //Mask the bits to obtain the 3 most significant bits. These can then be used to parse the request to the MBC
	{
	case 0x0000:
		// Toggle Ram
		if (cart->GetRamSize() > 0)
		{
			RAM_Enabled = ((data & 0x0F) == 0x0A);
		}
		break;
	case 0x2000:
		// Change rom bank
		if (currentMode == 0)
		{
			currentBank = (data & 0x1F) | (bankHigh << 5);
		}
		else
		{
			currentBank = data & 0x1f;
		}
		// Load memory bank 1
		memcpy(&memoryBus[0x4000], GetRomBank1(), 0x4000);
		break;
	case 0x4000:
		if (currentMode == 1)
		{
			ui8 ramBank = address & 0x03;
			ramBank &= (cart->RamBankCount() - 1);
			currentRAMBank = ramBank * 0x2000;
			// Load memory bank 1
			memcpy(&memoryBus[0xA000], &cart->GetRAMData()[(address - 0xA000) + currentRAMBank], 0x2000);
		}
		else
		{
			bankHigh = data & 0x03;

		}
		break;
	case 0x6000:
		currentMode = data & 0x01;
		break;
	case 0xA000:
		if (RAM_Enabled)
		{
			memoryBus[address] = cart->GetRAMData()[(address - 0xA000) + currentRAMBank] = data;
		}
		break;
	}
}
