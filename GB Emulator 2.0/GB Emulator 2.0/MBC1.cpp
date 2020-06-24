#include "MBC1.h"

MBC1::MBC1(Cartridge* cart, ui8* bus) : MBC(cart, bus)
{
	m_memory_bank = 1;
	m_mode = 0;
	m_rom_bank_high = 0;
	m_ram_offset = 0;
	m_ram_enabled = false;
}

ui8* MBC1::GetRomBank0()
{
	return &m_cart->GetRawRomMemory()[0x0000];
}

ui8* MBC1::GetRomBank1()
{
	return &m_cart->GetRawRomMemory()[0x4000 * m_memory_bank];
}

void MBC1::Write(ui16 address, ui8 data)
{
	switch (address & 0xE000)
	{
	case 0x0000:
		// Toggle Ram
		if (m_cart->GetRamSize() > 0)
			m_ram_enabled = ((data & 0x0F) == 0x0A);
		break;
	case 0x2000:
		// Change rom bank
		if (m_mode == 0)
		{
			m_memory_bank = (data & 0x1F) | (m_rom_bank_high << 5);
		}
		else
		{
			m_memory_bank = data & 0x1f;
		}

		// Load memory bank 1
		memcpy(&m_bus[0x4000], GetRomBank1(), 0x4000);
		break;
	case 0x4000:
		if (m_mode == 1)
		{
			ui8 ramBank = address & 0x03;
			ramBank &= (m_cart->RamBankCount() - 1);
			m_ram_offset = ramBank * 0x2000;
			// Load memory bank 1
			memcpy(&m_bus[0xA000], &m_cart->GetRawRamMemory()[(address - 0xA000) + m_ram_offset], 0x2000);
		}
		else
		{
			m_rom_bank_high = data & 0x03;

		}

		break;
	case 0x6000:
		if (m_cart->GetRamSize() != 3 && data & 0x01)
		{
			//assert(0 && "Unable to change MBC1 to mode 1. Not enough ram banks!");
		}

		m_mode = data & 0x01;
		break;
	case 0xA000:
		if (m_ram_enabled)
		{

			if (m_cart->GetRamSize() == 1 && address >= 0xA800)
			{
				assert(0 && "Attempting to wright out of ram's 2k memory range!");
			}

			m_bus[address] = m_cart->GetRawRamMemory()[(address - 0xA000) + m_ram_offset] = data;
		}
		else
		{
			//assert(0 && "Ram not enabled");
		}

		break;
	}
}
