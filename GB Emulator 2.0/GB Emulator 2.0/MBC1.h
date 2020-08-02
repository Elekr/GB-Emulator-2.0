#pragma once

#include "Cartridge.h"

#include <assert.h>

class MBC1 : public MBC
{
public:
	ui8 currentBank;
	ui8 bankHigh;
	ui8 currentMode;
	ui16 currentRAMBank;
	bool RAM_Enabled;

	MBC1(Cartridge* cart, ui8* bus);

	ui8* GetRomBank0();
	ui8* GetRomBank1();

	virtual void Write(ui16 address, ui8 data);
};

