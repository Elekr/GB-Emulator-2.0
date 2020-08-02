#pragma once

#include "MBC.h"

class RomOnly : public MBC
{
public:

	ui8 Bank1;

	RomOnly(Cartridge* cart, ui8* bus);

	ui8* GetRomBank0();
	ui8* GetRomBank1();

	virtual void Write(ui16 address, ui8 data);
};

