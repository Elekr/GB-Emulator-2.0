#pragma once

#include "MBC.h"

class RomOnly : public MBC
{
public:

	ui8 m_memory_bank;
	ui8 m_rom_bank_high;
	ui8 m_mode;
	int m_ram_bank;
	ui16 m_ram_offset;
	bool m_ram_enabled;


	RomOnly(Cartridge* cart, ui8* bus);

	ui8* GetRomBank0();
	ui8* GetRomBank1();

	virtual void write(ui16 address, ui8 data);
};

