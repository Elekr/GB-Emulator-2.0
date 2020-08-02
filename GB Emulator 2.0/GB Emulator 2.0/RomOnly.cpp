#pragma once

#include "RomOnly.h"
#include "Cartridge.h"

#include <assert.h>

RomOnly::RomOnly(Cartridge* cart, ui8* bus) : MBC(cart, bus) 
{

}

ui8* RomOnly::GetRomBank0()
{
	return &cart->GetROMData()[0x0000]; 
}

ui8* RomOnly::GetRomBank1()
{
	return &cart->GetROMData()[0x4000 * 1]; 
}

void RomOnly::Write(ui16 address, ui8 data)
{
	//ROM Only never writes to the ROM (Definition needed because virtual)
}
