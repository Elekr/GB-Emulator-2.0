#pragma once

#include "RomOnly.h"
#include "Cartridge.h"

#include <assert.h>

RomOnly::RomOnly(Cartridge* cart, ui8* bus) : MBC(cart, bus) 
{
	m_memory_bank = 1;
	m_mode = 0;
	m_rom_bank_high = 0;
	m_ram_bank = 0;
	m_ram_offset = 0;
	m_ram_enabled = false;
}


ui8* RomOnly::GetRomBank0()
{
	return &m_cart->GetRawRomMemory()[0x0000]; 
}

ui8* RomOnly::GetRomBank1()
{
	return &m_cart->GetRawRomMemory()[0x4000 * m_memory_bank]; //Get the current Bank TODO: change for ROM only?
}

void RomOnly::write(ui16 address, ui8 data)
{
	switch (address & 0xE000)
	{
	case 0x0000:
		// Toggle Ram
		if (m_cart->GetRamSize() > 0)
			m_ram_enabled = ((data & 0x0F) == 0x0A);
		break;
	case 0xA000:
		if (m_ram_enabled)
		{

			if (m_cart->GetRamSize() == 1 && address >= 0xA800)
			{
				assert(0 && "Attempting to write out of ram's 2k memory range!");
			}

			m_bus[address] = m_cart->GetRawRamMemory()[(address - 0xA000) + m_ram_offset] = data;
		}
		else
		{
			assert(0 && "Ram not enabled");
		}

	}
}
