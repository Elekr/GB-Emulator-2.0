#include "GB.h"
#include <assert.h>
#include <iomanip>
#include <bitset>

GB::GB()
{
	InitOPArray();
	InitCBOPArray();

	//Reset function
	Reset();
}

bool GB::InitEMU(const char* path)
{
	bool loaded = m_cartridge.Load(path);
	if (!loaded)return false;
	memcpy(m_bus, m_cartridge.GetRawData(), 0x8000);
	return true;
}

void GB::addBIOS()
{
	//PUT THE BIOS INTO THE M_BUS
	for (int i = 0; i < 256; i++)
	{
		m_bus[i] = BIOS[i];
	}
}

void GB::Reset()
{
	SetWordRegister(SP_REGISTER, 0x0000);
	SetPC(0x0000);

	SetWordRegister(AF_REGISTER, 0x0000);
	SetWordRegister(BC_REGISTER, 0x0000);
	SetWordRegister(DE_REGISTER, 0x0000);
	SetWordRegister(HL_REGISTER, 0x0000);



	for (unsigned int i = 0; i < m_display_buffer_size; i++)
	{
		frameBuffer[i] = 0x0;
	}

	for (int i = 0; i < BIOS_SIZE; i++)
	{
		m_bus[i] = BIOS[i];
	}

	for (int i = 0x9800; i < 0x9800 + 0x400; i++)
	{
		m_bus[i] = 0x0;
	}

	ReadData(LYRegister) = 144;

	interruptsEnabled = false;
	ReadData(m_cpu_interupt_flag_address) = 0x0;

	// Reset the joypad
	joypadActual = 0xFF;
	m_joypadCycles = 0;

	// Reset display
	ReadData(LYRegister) = 144; // Set scanline to 144
	videoCycles = 0;
	currentMode = V_BLANK;

	ClockFrequency();
}

void GB::WriteData(ui16 address, ui8 data)
{
	if (InMemoryRange(0x8000, 0x9FFF, address)) // High Frequancy
	{
		m_bus[address] = data;
		return;
	}

	// Main Ram
	if (InMemoryRange(0xC000, 0xDFFF, address)) // High Frequancy
	{
		m_bus[address] = data;
		return;
	}

	// Echo Ram
	if (InMemoryRange(0xE000, 0xFDFF, address)) // High Frequancy
	{
		// Since we are not supposed to read or write to this space, we shall access the main ram instead
		m_bus[address - 0x2000] = data;
		return;
	}

	// I/O
	if (InMemoryRange(0xFF00, 0xFF7F, address)) // High Frequancy
	{
		switch (address)
		{
		case 0xFF00: // Input
		{
			m_bus[address] = data;
			UpdateJoyPad();
			break;
		}
		case 0xFF04: // Timer divider
		{
			m_bus[address] = 0x0;
			break;
		}
		case 0xFF07: // Timer Control
		{
			SetTimerControl(data);
			break;
		}
		case 0xFF40: // Video Control
		{
			m_bus[address] = data;
			if (HasBit(data, 7))
			{
				EnableLCD();
			}
			else
			{
				DisableLCD();
			}

			
			break;
		}
		case 0xFF44: // Video Line Val. Reset if wrote too
		{
			m_bus[address] = 0;
			break;
		}
		case 0xFF46: // Transfer Sprites DMA
		{
			DMATransfer(data);
			break;
		}
		case 0xFF50: // Boot rom switch
		{
				if (data == 0)
				{
					for (int i = 0; i < BIOS_SIZE; i++)
					{
						m_bus[i] = BIOS[i];
					}
				}
				else
				{
					std::cout << "Booted Successfully" << std::endl;
					//DEBUGGING = true;
				/*	std::cout << std::hex << (int)m_bus[0x2f0] << std::endl;*/

					//ui8* cartridgeMemory = m_cartridge.GetRawData();
					//for (int i = 0; i < BIOS_SIZE; i++)
					//{
					//	m_bus[i] = cartridgeMemory[i];
					//}

					memcpy(m_bus, m_cartridge.GetRawData(), BIOS_SIZE);

					//DEBUGGING Display the first 256 bytes to see if bios has been overwritten
					//for (int i = 0; i < BIOS_SIZE; i++)
					//{
					//	std::cout << std::hex << i << " " << std::hex << (int)m_bus[i] << std::endl;
					//}

				}
			m_bus[address] = data;
			break;
		}
		default:
		{
			m_bus[address] = data;
		}
		}
		return;
	}

	// Cart Ram
	if (InMemoryRange(0xA000, 0xBFFF, address)) // Med Frequancy
	{
		// To do: Cart Ram
		return;
	}

	// OAM - Object Attribute Memory
	if (InMemoryRange(0xFE00, 0xFE9F, address)) // Med Frequancy
	{
		m_bus[address] = data;
		return;
	}

	// Cartridge ROM
	if (InMemoryRange(0x0000, 0x7FFF, address)) // Low Frequancy
	{
		// To do: MBC Rule change
		return;
	}

	// Interrupt https://www.reddit.com/r/EmuDev/comments/6sxb09/gb_tetris_stuck_at_copyright_screen/
	if (address == 0xFFFF) // Low Frequancy
	{
		m_bus[address] = data;
		return;
	}

	// Other uncaught Write commands
	m_bus[address] = data;
}

ui8& GB::ReadData(ui16 address)
{
	return m_bus[address];
}

bool GB::InMemoryRange(ui16 start, ui16 end, ui16 address)
{
	return start <= address && end >= address;
}

ui8 GB::ReadByte()
{
	ui8 byte = *dynamicPtr; //Gets the byte that the pointer is currently at
	cycle += 4;
	IncrementPC(); //Move along the pointer
	return byte;
}

ui16 GB::ReadWord()
{
	union //Share memory address for two 8-bit and 16-bit
	{
		ui16 data;
		struct //Separates the values of the 16-bit int
		{
			ui8 lowByte;
			ui8 highByte;
		};

	};
	//Sets the bytes
	lowByte = ReadByte();
	highByte = ReadByte();

	return data; //returns the Word
}

ui8& GB::GetByteRegister(ui8 reg)
{
	return register8bit[reg];
}

ui16& GB::GetWordRegister(ui8 reg)
{
	return register16bit[reg];
}

//** SET
void GB::SetByteRegister(ui8 reg, ui8 value)
{
	register8bit[reg] = value; //sets the 8-bit register
}

void GB::SetWordRegister(ui8 reg, ui16 value)
{
	register16bit[reg] = value; //sets the 16bit register
}


ui8 GB::ReadNextCode()
{
	ui8 result = *dynamicPtr;
	cycle += 4;
	IncrementPC();
	//TODO: HALT BUG STUFF
	return result;
}

void GB::IncrementPC()
{
	GetWordRegister(PC_REGISTER)++; //Increments the PC register by 1
	dynamicPtr++; //Moves the busPtr along one
}

inline void GB::SetPC(const ui16& value)
{
	register16bit[PC_REGISTER] = value;
	dynamicPtr = &m_bus[value];

	cycle += 4;
}

bool GB::HasBit(ui8& data, ui8 bit)
{
	return (data >> bit) & 1;
}

void GB::SetBit(ui8& data, ui8 bit)
{
	data |= 0x1 << bit;
}

void GB::ClearBit(ui8& data, ui8 bit)
{
	data &= ~(1 << bit);
}

void GB::PushStack(ui8 reg)
{
	union //Share memory address for two 8-bit and 16-bit
	{
		ui16 word;
		struct //Separates the values of the 16-bit int
		{
			ui8 lowByte;
			ui8 highByte;
		};

	};

	word = GetWordRegister(reg);
	GetWordRegister(SP_REGISTER)--; // Reduce the stack pointer before adding PC to SP
	WriteData(GetWordRegister(SP_REGISTER), highByte);
	GetWordRegister(SP_REGISTER)--;
	WriteData(GetWordRegister(SP_REGISTER), lowByte);

	cycle += 8;
}

void GB::PopStack(ui8 reg)
{
	union //Share memory address for two 8-bit and 16-bit
	{
		ui16 word;
		struct //Separates the values of the 16-bit int
		{
			ui8 lowByte;
			ui8 highByte;
		};

	};
	//Pop the word off the Stack
	lowByte = ReadData(GetWordRegister(SP_REGISTER));
	GetWordRegister(SP_REGISTER)++;
	highByte = ReadData(GetWordRegister(SP_REGISTER));
	GetWordRegister(SP_REGISTER)++;
	SetWordRegister(reg, word);

	cycle += 8;
}

void GB::PopStackPC()
{
	union
	{
		ui16 data;
		struct
		{
			ui8 low;
			ui8 high;
		};
	};
	low = ReadData(GetWordRegister(SP_REGISTER));
	GetWordRegister(SP_REGISTER)++;
	high = ReadData(GetWordRegister(SP_REGISTER));
	GetWordRegister(SP_REGISTER)++;
	//SetWordRegister(PC_REGISTER, data);

	cycle += 8;

	SetPC(data);
}

void GB::INCByteRegister(const ui8& reg) //Increments register and sets flags (Z, S, H)
{
	bool hasCarry = CheckFlag(FLAG_CARRY);

	ClearFlags();

	GetByteRegister(reg)++;

	SetFlag(FLAG_CARRY, hasCarry);

	SetFlag(FLAG_ZERO, GetByteRegister(reg) == 0);

	SetFlag(FLAG_HALFCARRY, (GetByteRegister(reg) & 0x0F) == 0x00);

	//http://www.z80.info/z80syntx.htm#INC <- helpful information on INC
	//https://stackoverflow.com/questions/8868396/gbz80-what-constitutes-a-half-carry HOW HALF CARRY WORKS
	//https://robdor.com/2016/08/10/gameboy-emulator-half-carry-flag/ //Explanation
	//https://github.com/taisel/GameBoy-Online/blob/master/js/GameBoyCore.js#L588 //Lazy way

}

void GB::DECByteRegister(const ui8& reg)
{
	GetByteRegister(reg)--;

	if (!CheckFlag(FLAG_CARRY))
	{
		ClearFlags();
	}

	SetFlag(FLAG_ZERO, GetByteRegister(reg) == 0);
	SetFlag(FLAG_SUBTRACT, true);
	SetFlag(FLAG_HALFCARRY, (GetByteRegister(reg) & 0x0F) == 0x0F);
}

void GB::SetFlag(int flag, bool value)
{
	if (value)
	{
		GetByteRegister(F_REGISTER) |= 1 << flag; // Use OR (|) on the flag bit ( 1|0 = 1)
	}
	else
	{
		GetByteRegister(F_REGISTER) &= ~(1 << flag); // Reverse the bits using NOT (~) then use AND (&)
	}
}

bool GB::CheckFlag(int flag)
{
	return (GetByteRegister(F_REGISTER) >> flag) & 1; // Shift the number to the right then AND mask it to 1
}

void GB::ClearFlags() /////////
{
	GetByteRegister(F_REGISTER) = 0x00;
}

void GB::Jr()
{
	ui16 currentPC = GetWordRegister(PC_REGISTER); //Get the current PC value
	ui16 newPC = currentPC + 1 + static_cast<i8>(ReadByte());
	SetPC(newPC);
}

void GB::Jr(const ui8& address)
{
	SetPC(address);
}

void GB::ADDHL(const ui16& reg)
{
	int result = GetWordRegister(HL_REGISTER) + reg;

	bool hasZero = CheckFlag(FLAG_ZERO);

	ClearFlags();
	
	SetFlag(FLAG_ZERO, hasZero);

	if (result & 0x10000)
	{
		SetFlag(FLAG_CARRY, true);
	}

	if ((GetWordRegister(HL_REGISTER) ^ reg ^ (result & 0xFFFF)) & 0x1000)
	{
		SetFlag(FLAG_HALFCARRY, true);
	}

	SetWordRegister(HL_REGISTER, static_cast<ui16>(result));

	cycle += 4;
}

void GB::Bit(const ui8& value, ui8 bit)
{
	ClearFlags();

	SetFlag(FLAG_ZERO, ((value >> bit) & 0x01) == 0);
	SetFlag(FLAG_HALFCARRY, true);
}

void GB::XOR(const ui8& value)
{
	ui8 result = GetByteRegister(A_REGISTER) ^ value;

	SetByteRegister(A_REGISTER, result);

	ClearFlags();
	SetFlag(FLAG_ZERO, (result == 0));

}

void GB::OR(const ui8& value)
{
	ui8 a = GetByteRegister(A_REGISTER);
	ui8 result = a | value;

	SetByteRegister(A_REGISTER, result);

	SetFlag(FLAG_ZERO, result == 0);
	SetFlag(FLAG_SUBTRACT, false);
	SetFlag(FLAG_HALFCARRY, false);
	SetFlag(FLAG_CARRY, false);
}

void GB::LDI(const ui16& address, const ui8& reg)
{
	WriteData(address, GetByteRegister(reg));
	GetWordRegister(HL_REGISTER)++;

	cycle += 4;
}

void GB::LDI(const ui8& reg, const ui16& address)
{
	SetByteRegister(reg, ReadData(address));
	GetWordRegister(HL_REGISTER)++;
}

void GB::RL(ui8& reg, bool A)
{
	//http://z80-heaven.wikidot.com/instructions-set:rl how RL works

	ui8 carry = CheckFlag(FLAG_CARRY) ? 1 : 0;

	ClearFlags();

	if ((reg & 0x80) != 0) //  If a Carry has happened
	{
		SetFlag(FLAG_CARRY, true);
	}

	reg <<= 1;

	reg |= carry;

	if (!A) // If RL, N variant

	{
		SetFlag(FLAG_ZERO, (reg == 0));
	}
}

void GB::RLC(ui8& reg, bool A)
{
	//Similar to the RL function but sets the 7th bit to the carry

	ClearFlags();

	if ((reg & 0x80) != 0) // if it isn't 0 must be 1 (so carry)
	{
		SetFlag(FLAG_CARRY, true);
		reg <<= 1;
		reg |= 0x1;
	}
	else
	{
		reg <<= 1;
	}

	if (!A)
	{
		SetFlag(FLAG_ZERO, (reg == 0));
	}
}

void GB::RR(ui8& reg, bool A)
{
	ui8 carry = CheckFlag(FLAG_CARRY) ? 0x80 : 0x00;

	ClearFlags();
	if ((reg & 0x01) != 0)
	{
		SetFlag(FLAG_CARRY, true);
	}

	reg >>= 1;
	reg |= carry;

	if (!A)
	{
		SetFlag(FLAG_ZERO, (reg == 0));
	}
}

void GB::RRC(ui8& reg, bool A)
{
	ClearFlags();

	if ((reg & 0x01) != 0)
	{
		SetFlag(FLAG_CARRY, true);
		reg >>= 1;
		reg |= 0x80;
	}
	else
	{
		reg >>= 1;
	}

	if (!A)
	{
		SetFlag(FLAG_ZERO, (reg == 0));
	}
}

void GB::CP(const ui8& value)
{
	ui8 reg = GetByteRegister(A_REGISTER);

	SetFlag(FLAG_SUBTRACT, true);
	SetFlag(FLAG_CARRY, reg < value);
	SetFlag(FLAG_ZERO, reg == value);
	SetFlag(FLAG_HALFCARRY, ((reg - value) & 0xF) > (reg & 0xF));
}

void GB::SUB(const ui8& value)
{
	int current_register = GetByteRegister(A_REGISTER);
	int result = current_register - value;
	int carrybits = current_register ^ value ^ result;

	SetByteRegister(A_REGISTER, static_cast<ui8>(result));

	ClearFlags();

	SetFlag(FLAG_SUBTRACT, true);

	SetFlag(FLAG_ZERO, static_cast<ui8>(result) == 0);

	if ((carrybits & 0x100) != 0)
	{
		SetFlag(FLAG_CARRY, true);
	}
	if ((carrybits & 0x10) != 0)
	{
		SetFlag(FLAG_HALFCARRY, true);
	}
}

void GB::SBC(const ui8& value)
{
	ui8 a_reg = GetByteRegister(A_REGISTER);

	int carry = CheckFlag(FLAG_CARRY) ? 1 : 0;
	int result = a_reg - value - carry;

	ClearFlags();
	SetFlag(FLAG_SUBTRACT, true);
	SetFlag(FLAG_ZERO, static_cast<ui8>(result) == 0);
	if (result < 0)
	{
		SetFlag(FLAG_CARRY, true);
	}

	if (((a_reg & 0x0F) - (value & 0x0F) - carry) < 0)
	{
		SetFlag(FLAG_HALFCARRY, true);
	}


	SetByteRegister(A_REGISTER, static_cast<ui8> (result));
}

void GB::ADD(const ui8& reg, const ui8& value)
{
	int result = GetByteRegister(reg) + value;
	int carrybits = GetByteRegister(reg) ^ value ^ result;

	SetByteRegister(A_REGISTER, static_cast<ui8> (result));


	SetFlag(FLAG_ZERO, GetByteRegister(A_REGISTER) == 0);
	SetFlag(FLAG_CARRY, (carrybits & 0x100) != 0);
	SetFlag(FLAG_HALFCARRY, (carrybits & 0x10) != 0);
	SetFlag(FLAG_SUBTRACT, false);
}

void GB::ADC(const ui8& value)
{
	int carry = CheckFlag(FLAG_CARRY) ? 1 : 0;
	int result = GetByteRegister(A_REGISTER) + value + carry;

	SetFlag(FLAG_ZERO, static_cast<ui8> (result) == 0);
	SetFlag(FLAG_CARRY, result > 0xFF);
	SetFlag(FLAG_HALFCARRY, ((GetByteRegister(A_REGISTER) & 0x0F) + (value & 0x0F) + carry) > 0x0F);
	SetFlag(FLAG_SUBTRACT, false);

	SetByteRegister(A_REGISTER, static_cast<ui8> (result));
}

void GB::AND(const ui8& value)
{
	ui8 result = GetByteRegister(A_REGISTER) & value;

	SetByteRegister(A_REGISTER, result);

	ClearFlags();
	SetFlag(FLAG_ZERO, result == 0);
	SetFlag(FLAG_HALFCARRY, true);
}

void GB::SLA(ui8& value)
{
	ClearFlags();
	if ((value & 0x80) != 0)
	{
		SetFlag(FLAG_CARRY, true);
	}

	value <<= 1;
	SetFlag(FLAG_ZERO, value == 0);
}

void GB::SRA(ui8& value)
{
	ClearFlags();
	if ((value & 0x01) != 0)
	{
		SetFlag(FLAG_CARRY, true);
	}

	if ((value & 0x80) != 0)
	{
		value >>= 1;
		value |= 0x80;
	}
	else
	{
		value >>= 1;
	}

	SetFlag(FLAG_ZERO, value == 0);
}

void GB::Swap(ui8& value)
{
	ui8 low_half = value & 0x0F;
	ui8 high_half = (value >> 4) & 0x0F;

	value = (low_half << 4) + high_half;


	SetFlag(FLAG_ZERO, value == 0);
	SetFlag(FLAG_CARRY, false);
	SetFlag(FLAG_HALFCARRY, false);
	SetFlag(FLAG_SUBTRACT, false);
}

void GB::SRL(ui8& value)
{
	ClearFlags();
	if ((value & 0x01) != 0)
	{
		SetFlag(FLAG_CARRY, true);
	}
	value >>= 1;
	SetFlag(FLAG_ZERO, value == 0);
}

void GB::NextFrame()
{
	while(!TickCPU());
	cycles = 0;
	RenderGame();
}

bool GB::TickCPU()
{
	cycle = 0;

	if (halt)
	{
		cycle = 4;

		if (m_haltDissableCycles > 0)
		{
			m_haltDissableCycles -= cycle;

			if (m_haltDissableCycles <= 0)
			{
				m_haltDissableCycles == 0;
				halt = false;
			}

		}
		else
		{
			ui8& interupt_flags = ReadData(m_cpu_interupt_flag_address);
			ui8& interupt_enabled_flags = ReadData(m_interrupt_enabled_flag_address);

			ui8 interuptsToProcess = interupt_flags & interupt_enabled_flags;

			if (halt && interuptsToProcess > 0)
			{
				m_haltDissableCycles = 16;
			}
		}
	}

	if (!halt)
	{
		OPCode = ReadNextCode();
		CheckInterrupts();


		cycle = (normalCycles[OPCode] * 4);
		cycles += cycle;

		int address1 = 0xC50f;
		int address2 = 0xc5f8;

		//if (GetWordRegister(PC_REGISTER) == address1 /*&& GetWordRegister(PC_REGISTER) <= address2*/)
		//{
		//	DEBUGGING = true;
		//}

		if (OPCode == 0xCB) // Extra Codes
		{
			OPCode = ReadNextCode();

			cycle = (CBCycles[OPCode] * 4);
			cycles += cycle;

			(this->*CBCodes[OPCode])();
			if (DEBUGGING)
			{
				if (GetWordRegister(PC_REGISTER) >= address1 && GetWordRegister(PC_REGISTER) <= address2)
				{
					OUTPUTCBREGISTERS(OPCode);
					std::cout << std::hex << (int)GetWordRegister(PC_REGISTER) << std::endl;
				}

				
				//OUTPUTCBREGISTERS(OPCode);
			}

		}
		else
		{
			(this->*BASECodes[OPCode])();
			if (DEBUGGING)
			{
				if (GetWordRegister(PC_REGISTER) >= address1 && GetWordRegister(PC_REGISTER) <= address2)
				{
					OUTPUTREGISTERS(OPCode);
					std::cout << std::hex << (int)GetWordRegister(PC_REGISTER) << std::endl;

				}

			
				//OUTPUTCBREGISTERS(OPCode);
			}

		}
	}

	
	TickClock();
	bool vSync = updatePixels();
	//JoyPadTick();
	return vSync;
}

void GB::TickClock()
{
	divCounter += cycles;

	ui8 timer_control = ReadData(m_timer_controll_address);
	ui8 timer = ReadData(m_timer_address);
	ui8 timer_modulo = ReadData(m_timer_modulo_address);

	if (HasBit(timer_control, 2)) //If the timer is enabled
	{
		timerCounter += cycle;

		ClockFrequency();

		//Incrementing the timer register
		while (timerCounter >= clockFreq)
		{
			timerCounter -= clockFreq;
			if (timer == 0xFF)
			{
				timer = timer_modulo;

				RequestInterupt(TIMER);
			}
			else
			{
				timer++;
			}
		}
	}

	//Incrementing the divider register
	while (divCounter >= 256)
	{
		divCounter -= 256;

		ReadData(m_timer_divider_address)++;
	}

	// 00: CPU Clock  /1024  (DMG, CGB:   4096 Hz,   SGB : ~4194 Hz)
	// 01 : CPU Clock /16    (DMG, CGB:   262144 Hz, SGB : ~268400 Hz)
	// 10 : CPU Clock /64    (DMG, CGB:   65536 Hz,  SGB : ~67110 Hz)
	// 11 : CPU Clock /256   (DMG, CGB:   16384 Hz,  SGB : ~16780 Hz)
}

void GB::ClockFrequency()
{
	switch (ReadData(m_timer_controll_address) & 0x03)
	{
	case 0:
		clockFreq = 1024; // Frequency 4096
		break;
	case 1:
		clockFreq = 16; // Frequency 262144
		break;
	case 2:
		clockFreq = 64; // Frequency 65536
		break;
	case 3:
		clockFreq = 256; // Frequency 16382
		break;
	}
}

void GB::SetTimerControl(ui8 data)
{
	ui8 current = ReadData(m_timer_controll_address);
	ReadData(m_timer_controll_address) = data;
	ui8 new_data = ReadData(m_timer_controll_address);
	if (new_data != current)
	{
		ClockFrequency();
	}
}

void GB::RequestInterupt(CPUInterupt interupt)
{
	ReadData(m_cpu_interupt_flag_address) |= 1 << (ui8)interupt;
}

void GB::UpdateLCDStatus()
{
	ui8& status = ReadData(0xFF41);
	// Set mode to memory //what is this i don't know
	status = (status & 0xFC);
}

void GB::CheckInterrupts()
{
	ui8& interupt_flags = ReadData(m_cpu_interupt_flag_address);
	ui8& interupt_enabled_flags = ReadData(m_interrupt_enabled_flag_address);

	ui8 interuptsToProcess = interupt_flags & interupt_enabled_flags; //figure out how this works
	if (DEBUGGING)
	{
		/*std::cout << "Interrupts flags" << std::bitset<8>((int)interupt_flags) << std::endl;
		std::cout << "Interrupts enabled" << std::bitset<8>((int)interupt_flags) << std::endl;*/
		//std::cout << "Interrupts to process" <<  (int)interuptsToProcess << std::endl;
	}


	if (interuptsToProcess > 0)
	{
		if (interruptsEnabled)
		{
			// Loop through for all possible interrupts 
			for (int i = 0; i < 5; i++)
			{
				if (HasBit(interupt_flags, i))
				{
					interruptsEnabled = false; // Since we are now in a interrupt we need to disable future ones
					halt = false;
					ClearBit(interupt_flags, i);
					PushStack(PC_REGISTER); //Store PC
					switch (i)
					{
					case 0: // VBlank
						SetPC(0x0040);
						break;
					case 1: // LCDStat
						SetPC(0x0048);
						break;
					case 2: // Timer
						SetPC(0x0050);
						break;
					case 3: // Serial
						SetPC(0x0058);
						break;
					case 4: // Joypad
						SetPC(0x0060);
						break;
					default:
						assert(0 && "Unqnown interupt");
						break;
					}

					cycle += 20;

					return;
				}
			}
		}
		else if (halt)
		{
			halt = false;
		}

	}
}

void GB::CompareLYWithLYC()
{
	ui8 controlBit = ReadData(lcdcRegister); // Get the LCDC register from the CPU
	ui8 line = ReadData(LYRegister);

	bool isDisplayEnabled = HasBit(controlBit, 7);

	if (isDisplayEnabled)
	{
		ui8 status = ReadData(statusRegister);
		ui8 LYC = ReadData(lycRegister);

		if (LYC == line)
		{
			SetBit(status, 2);
			if (HasBit(status, 6))
			{
				RequestInterupt(LCD);
			}
		}
		else
		{
			ClearBit(status, 2);
		}
	}
}

//OP CODES
void GB::OP00() {}; // NOP
void GB::OP01() { SetWordRegister(BC_REGISTER, ReadWord()); }; // LD BC, nn 
void GB::OP02() { WriteData(GetWordRegister(BC_REGISTER), GetByteRegister(A_REGISTER));  cycle += 4; }; // LD (BC), A
void GB::OP03() { GetWordRegister(BC_REGISTER)++; cycle += 4; }; // INC BC
void GB::OP04() { INCByteRegister(B_REGISTER); }; // INC B
void GB::OP05() { DECByteRegister(B_REGISTER); }; // DEC B
void GB::OP06() { SetByteRegister(B_REGISTER, ReadByte()); }; // LD B, ui8
void GB::OP07() { RLC(GetByteRegister(A_REGISTER), true); }; // RLCA
void GB::OP08() 
{
	ui16 pc = ReadWord();

	union
	{
		ui16 data;
		struct
		{
			ui8 low;
			ui8 high;
		};
	};
	data = GetWordRegister(SP_REGISTER);
	cycle += 8;
	WriteData(pc, low);
	WriteData(pc + 1, high);
}; // LD (nn) SP
void GB::OP09() { ADDHL(GetWordRegister(BC_REGISTER)); }; // ADD HL, BC
void GB::OP0A() { SetByteRegister(A_REGISTER, ReadData(GetWordRegister(BC_REGISTER)));  cycle += 4; }; // LD A, (BC)
void GB::OP0B() { GetWordRegister(BC_REGISTER)--; cycle += 4; }; // DEC BC
void GB::OP0C() { INCByteRegister(C_REGISTER); }; // INC C
void GB::OP0D() { DECByteRegister(C_REGISTER); }; // DEC C
void GB::OP0E() { SetByteRegister(C_REGISTER, ReadByte()); }; // LD C, ui8
void GB::OP0F() { RRC(GetByteRegister(A_REGISTER), true); }; // RRCA
void GB::OP10() { IncrementPC(); }; // STOP (does nothing unless gbc)
void GB::OP11() { SetWordRegister(DE_REGISTER, ReadWord()); }; // LD DE, nn
void GB::OP12() { WriteData(GetWordRegister(DE_REGISTER), GetByteRegister(A_REGISTER)); cycle += 4; }; // LD (DE), A
void GB::OP13() { GetWordRegister(DE_REGISTER)++; cycle += 4; }; // INC DE
void GB::OP14() { INCByteRegister(D_REGISTER); }; // INC D
void GB::OP15() { DECByteRegister(D_REGISTER); }; // DEC D
void GB::OP16() { SetByteRegister(D_REGISTER, ReadByte()); }; // LD D, ui8
void GB::OP17() { RL(GetByteRegister(A_REGISTER), true); }; // RLA
void GB::OP18() { Jr(); }; // Jr i8
void GB::OP19() { ADDHL(GetWordRegister(DE_REGISTER)); }; // ADD HL, DE
void GB::OP1A() { SetByteRegister(A_REGISTER, ReadData(GetWordRegister(DE_REGISTER))); cycle += 4; }; // LD A, (DE)
void GB::OP1B() { GetWordRegister(DE_REGISTER)--; cycle += 4; }; // DEC DE
void GB::OP1C() { INCByteRegister(E_REGISTER); }; // INC E
void GB::OP1D() { DECByteRegister(E_REGISTER); }; // DEC E
void GB::OP1E() { SetByteRegister(E_REGISTER, ReadByte()); }; // LD E, ui8
void GB::OP1F() { RR(GetByteRegister(A_REGISTER), true); }; // RRA
void GB::OP20() 
{
	if(!CheckFlag(FLAG_ZERO)) // check if the Zero flag is not set
	{
		Jr();
	}
	else
	{
		ReadByte(); //Removes the jump instruction
	}
}; // JR NZ, i8 
void GB::OP21() { SetWordRegister(HL_REGISTER, ReadWord()); }; // LD HL, nn
void GB::OP22() { LDI(GetWordRegister(HL_REGISTER), A_REGISTER); }; // LD (HL+), A
void GB::OP23() { GetWordRegister(HL_REGISTER)++; cycle += 4; }; // INC HL
void GB::OP24() { INCByteRegister(H_REGISTER); }; // INC H
void GB::OP25() { DECByteRegister(H_REGISTER); }; // DEC H
void GB::OP26() { SetByteRegister(H_REGISTER, ReadByte()); }; // LD H, ui8
void GB::OP27() 
{
	int a_reg = GetByteRegister(A_REGISTER);

	if (!CheckFlag(FLAG_SUBTRACT))
	{
		if (CheckFlag(FLAG_HALFCARRY) || ((a_reg & 0xF) > 9))
			a_reg += 0x06;

		if (CheckFlag(FLAG_CARRY) || (a_reg > 0x9F))
			a_reg += 0x60;
	}
	else
	{
		if (CheckFlag(FLAG_HALFCARRY))
			a_reg = (a_reg - 6) & 0xFF;

		if (CheckFlag(FLAG_CARRY))
			a_reg -= 0x60;
	}
	SetFlag(FLAG_HALFCARRY, false);

	if ((a_reg & 0x100) == 0x100)
		SetFlag(FLAG_CARRY, true);

	a_reg &= 0xFF;

	SetFlag(FLAG_ZERO, a_reg == 0);

	SetByteRegister(A_REGISTER, a_reg);
}; // DAA
void GB::OP28() 
{
	if (CheckFlag(FLAG_ZERO))
	{
		Jr();
	}
	else
	{
		ReadByte();
	}
}; // JR Z, i8
void GB::OP29() { ADDHL(GetWordRegister(HL_REGISTER)); }; // ADD HL, HL
void GB::OP2A() { LDI(A_REGISTER, GetWordRegister(HL_REGISTER)); cycle += 4; }; // LD A, (HL+)
void GB::OP2B() { GetWordRegister(HL_REGISTER)--; cycle += 4; }; // DEC HL
void GB::OP2C() { INCByteRegister(L_REGISTER); }; // INC L
void GB::OP2D() { DECByteRegister(L_REGISTER); }; // DEC L
void GB::OP2E() { SetByteRegister(L_REGISTER, ReadByte()); }; // LD L, ui8
void GB::OP2F() 
{
	SetByteRegister(A_REGISTER, ~GetByteRegister(A_REGISTER)); // Flips the bits of the register

	SetFlag(FLAG_HALFCARRY, true);
	SetFlag(FLAG_SUBTRACT, true);
}; // CPL	http://www.cplusplus.com/doc/tutorial/operators/
void GB::OP30() 
{
	if (!CheckFlag(FLAG_CARRY))
	{
		Jr();
	}
	else
	{
		ReadByte();
	}
}; // JR NC, i8 (Not Carry)
void GB::OP31() { SetWordRegister(SP_REGISTER, ReadWord()); }; // LD SP, nn
void GB::OP32() 
{
	WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(A_REGISTER));
	GetWordRegister(HL_REGISTER)--;
	cycle += 4;
}; // LD (HL--), A https://blog.tigris.fr/2019/07/28/writing-an-emulator-memory-management/
void GB::OP33() { GetWordRegister(SP_REGISTER)++;  cycle += 4; }; // INC SP
void GB::OP34() 
{
	ui8 byte = ReadData(GetWordRegister(HL_REGISTER)); // store the data
	byte++; // inc
	WriteData(GetWordRegister(HL_REGISTER), byte);

	bool hasCarry = CheckFlag(FLAG_CARRY);

	ClearFlags();

	SetFlag(FLAG_CARRY, hasCarry);
	SetFlag(FLAG_ZERO, byte == 0);
	SetFlag(FLAG_HALFCARRY, (byte & 0x0F) == 0x00);

	cycle += 8;

}; // INC (HL)
void GB::OP35()
{
	ui8 data = ReadData(GetWordRegister(HL_REGISTER));
	data--;
	WriteData(GetWordRegister(HL_REGISTER), data);

	if (!CheckFlag(FLAG_CARRY))
	{
		ClearFlags();
	}

	SetFlag(FLAG_ZERO, data == 0);
	SetFlag(FLAG_SUBTRACT, true);
	SetFlag(FLAG_HALFCARRY, (data & 0x0F) == 0x0F);

	cycle += 8;

} // return data back}; // DEC (HL)
void GB::OP36() { WriteData(GetWordRegister(HL_REGISTER), ReadByte()); cycle += 4; }; // LD (HL) ui8
void GB::OP37() 
{
	SetFlag(FLAG_CARRY, true);
	SetFlag(FLAG_HALFCARRY, false);
	SetFlag(FLAG_SUBTRACT, false);

}; // SCF http://z80-heaven.wikidot.com/instructions-set:scf
void GB::OP38() 
{
	if (CheckFlag(FLAG_CARRY))
	{
		Jr();
	}
	else
	{
		ReadByte();
	}
}; // JR C, i8
void GB::OP39() { ADDHL(GetWordRegister(SP_REGISTER)); }; // ADD HL, SP
void GB::OP3A() 
{
	SetByteRegister(A_REGISTER, ReadData(GetWordRegister(HL_REGISTER)));
	GetWordRegister(HL_REGISTER)--;
	cycle += 4;
}; // LD A, (HL-)
void GB::OP3B() { GetWordRegister(SP_REGISTER)--; cycle += 4; }; // DEC SP
void GB::OP3C() { INCByteRegister(A_REGISTER); }; // INC A
void GB::OP3D() { DECByteRegister(A_REGISTER); }; // DEC A
void GB::OP3E() { SetByteRegister(A_REGISTER, ReadByte()); }; // LD A, ui8
void GB::OP3F() 
{
	SetFlag(FLAG_CARRY, !CheckFlag(FLAG_CARRY));
	SetFlag(FLAG_HALFCARRY, false);
	SetFlag(FLAG_SUBTRACT, false);

}; // CCF http://z80-heaven.wikidot.com/instructions-set:ccf
void GB::OP40() { SetByteRegister(B_REGISTER, GetByteRegister(B_REGISTER)); }; // LD B, B
void GB::OP41() { SetByteRegister(B_REGISTER, GetByteRegister(C_REGISTER)); }; // LD B, C
void GB::OP42() { SetByteRegister(B_REGISTER, GetByteRegister(D_REGISTER)); }; // LD B, D
void GB::OP43() { SetByteRegister(B_REGISTER, GetByteRegister(E_REGISTER)); }; // LD B, E
void GB::OP44() { SetByteRegister(B_REGISTER, GetByteRegister(H_REGISTER)); }; // LD B, H
void GB::OP45() { SetByteRegister(B_REGISTER, GetByteRegister(L_REGISTER)); }; // LD B, L
void GB::OP46() { SetByteRegister(B_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // LD B, HL
void GB::OP47() { SetByteRegister(B_REGISTER, GetByteRegister(A_REGISTER)); }; // LD B, A
void GB::OP48() { SetByteRegister(C_REGISTER, GetByteRegister(B_REGISTER)); }; // LD C, B
void GB::OP49() { SetByteRegister(C_REGISTER, GetByteRegister(C_REGISTER)); }; // LD C, C
void GB::OP4A() { SetByteRegister(C_REGISTER, GetByteRegister(D_REGISTER)); }; // LD C, D
void GB::OP4B() { SetByteRegister(C_REGISTER, GetByteRegister(E_REGISTER)); }; // LD C, E
void GB::OP4C() { SetByteRegister(C_REGISTER, GetByteRegister(H_REGISTER)); }; // LD C, H
void GB::OP4D() { SetByteRegister(C_REGISTER, GetByteRegister(L_REGISTER)); }; // LD C, L
void GB::OP4E() { SetByteRegister(C_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // LD C, HL
void GB::OP4F() { SetByteRegister(C_REGISTER, GetByteRegister(A_REGISTER)); }; // LD C, A
void GB::OP50() { SetByteRegister(D_REGISTER, GetByteRegister(B_REGISTER)); }; // LD D,B
void GB::OP51() { SetByteRegister(D_REGISTER, GetByteRegister(C_REGISTER)); }; // LD D,C
void GB::OP52() { SetByteRegister(D_REGISTER, GetByteRegister(D_REGISTER)); }; // LD D,D
void GB::OP53() { SetByteRegister(D_REGISTER, GetByteRegister(E_REGISTER)); }; // LD D,E
void GB::OP54() { SetByteRegister(D_REGISTER, GetByteRegister(H_REGISTER)); }; // LD D,H
void GB::OP55() { SetByteRegister(D_REGISTER, GetByteRegister(L_REGISTER)); }; // LD D,L
void GB::OP56() { SetByteRegister(D_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // LD D, (HL)
void GB::OP57() { SetByteRegister(D_REGISTER, GetByteRegister(A_REGISTER)); }; //LD D, A
void GB::OP58() { SetByteRegister(E_REGISTER, GetByteRegister(B_REGISTER)); }; // LD E, B
void GB::OP59() { SetByteRegister(E_REGISTER, GetByteRegister(C_REGISTER)); }; // LD E, C
void GB::OP5A() { SetByteRegister(E_REGISTER, GetByteRegister(D_REGISTER)); }; // LD E, D
void GB::OP5B() { SetByteRegister(E_REGISTER, GetByteRegister(E_REGISTER)); }; // LD E, E
void GB::OP5C() { SetByteRegister(E_REGISTER, GetByteRegister(H_REGISTER)); }; // LD E, H
void GB::OP5D() { SetByteRegister(E_REGISTER, GetByteRegister(L_REGISTER)); }; // LD E, L
void GB::OP5E() { SetByteRegister(E_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // LD E, (HL)
void GB::OP5F() { SetByteRegister(E_REGISTER, GetByteRegister(A_REGISTER)); }; // LD E, A
void GB::OP60() { SetByteRegister(H_REGISTER, GetByteRegister(B_REGISTER)); }; // LD H, B
void GB::OP61() { SetByteRegister(H_REGISTER, GetByteRegister(C_REGISTER)); }; // LD H, C
void GB::OP62() { SetByteRegister(H_REGISTER, GetByteRegister(D_REGISTER)); }; // LD H, D
void GB::OP63() { SetByteRegister(H_REGISTER, GetByteRegister(E_REGISTER)); }; // LD H, E
void GB::OP64() { SetByteRegister(H_REGISTER, GetByteRegister(H_REGISTER)); }; // LD H, H
void GB::OP65() { SetByteRegister(H_REGISTER, GetByteRegister(L_REGISTER)); }; // LD H, L
void GB::OP66() { SetByteRegister(H_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // LD H, (HL)
void GB::OP67() { SetByteRegister(H_REGISTER, GetByteRegister(A_REGISTER)); }; // LD H, A
void GB::OP68() { SetByteRegister(L_REGISTER, GetByteRegister(B_REGISTER)); }; // LD L, B
void GB::OP69() { SetByteRegister(L_REGISTER, GetByteRegister(C_REGISTER)); }; // LD L, C
void GB::OP6A() { SetByteRegister(L_REGISTER, GetByteRegister(D_REGISTER)); }; // LD L, D
void GB::OP6B() { SetByteRegister(L_REGISTER, GetByteRegister(E_REGISTER)); }; // LD L, E
void GB::OP6C() { SetByteRegister(L_REGISTER, GetByteRegister(H_REGISTER)); }; // LD L, H
void GB::OP6D() { SetByteRegister(L_REGISTER, GetByteRegister(L_REGISTER)); }; // LD L, L
void GB::OP6E() { SetByteRegister(L_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // LD L, (HL)
void GB::OP6F() { SetByteRegister(L_REGISTER, GetByteRegister(A_REGISTER)); }; // LD L, A
void GB::OP70() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(B_REGISTER)); cycle += 4; }; // LD (HL), B
void GB::OP71() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(C_REGISTER)); cycle += 4; }; // LD (HL), C
void GB::OP72() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(D_REGISTER)); cycle += 4; }; // LD (HL), D
void GB::OP73() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(E_REGISTER)); cycle += 4; }; // LD (HL), E
void GB::OP74() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(H_REGISTER)); cycle += 4; }; // LD (HL), H
void GB::OP75() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(L_REGISTER)); cycle += 4; }; // LD (HL), L
void GB::OP76() 
{
	halt = true;

	/* Check for halt bug */
	ui8 interruptEnabledFlag = ReadData(m_interrupt_enabled_flag_address);
	ui8 interruptFlag = ReadData(m_cpu_interupt_flag_address);
}; // HALT
void GB::OP77() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(A_REGISTER)); cycle += 4; }; // LD (HL), A
void GB::OP78() { SetByteRegister(A_REGISTER, GetByteRegister(B_REGISTER)); }; // LD A, B
void GB::OP79() { SetByteRegister(A_REGISTER, GetByteRegister(C_REGISTER)); }; // LD A, C
void GB::OP7A() { SetByteRegister(A_REGISTER, GetByteRegister(D_REGISTER)); }; // LD A, D
void GB::OP7B() { SetByteRegister(A_REGISTER, GetByteRegister(E_REGISTER)); }; // LD A, E
void GB::OP7C() { SetByteRegister(A_REGISTER, GetByteRegister(H_REGISTER)); }; // LD A, H
void GB::OP7D() { SetByteRegister(A_REGISTER, GetByteRegister(L_REGISTER)); }; // A, L
void GB::OP7E() { SetByteRegister(A_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // LD A, (HL)
void GB::OP7F() { SetByteRegister(A_REGISTER, GetByteRegister(A_REGISTER)); }; // LD A, A
void GB::OP80() { ADD(A_REGISTER, GetByteRegister(B_REGISTER)); }; //ADD A, B
void GB::OP81() { ADD(A_REGISTER, GetByteRegister(C_REGISTER)); }; // ADD A, C
void GB::OP82() { ADD(A_REGISTER, GetByteRegister(D_REGISTER)); }; // ADD A, D
void GB::OP83() { ADD(A_REGISTER, GetByteRegister(E_REGISTER)); }; // ADD A, E
void GB::OP84() { ADD(A_REGISTER, GetByteRegister(H_REGISTER)); }; // ADD A, H
void GB::OP85() { ADD(A_REGISTER, GetByteRegister(L_REGISTER)); }; // ADD A, L
void GB::OP86() { ADD(A_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; //ADD A, HL
void GB::OP87() { ADD(A_REGISTER, GetByteRegister(A_REGISTER)); }; // ADD A, A
void GB::OP88() { ADC(GetByteRegister(B_REGISTER)); }; // ADC B
void GB::OP89() { ADC(GetByteRegister(C_REGISTER)); }; // ADC C
void GB::OP8A() { ADC(GetByteRegister(D_REGISTER)); }; // ADC D
void GB::OP8B() { ADC(GetByteRegister(E_REGISTER)); }; // ADC E
void GB::OP8C() { ADC(GetByteRegister(H_REGISTER)); }; // ADC H
void GB::OP8D() { ADC(GetByteRegister(L_REGISTER)); }; // ADC L
void GB::OP8E() { ADC(ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // ADC (HL)
void GB::OP8F() { ADC(GetByteRegister(A_REGISTER)); }; // ADC A
void GB::OP90() { SUB(GetByteRegister(B_REGISTER)); }; // SUB B
void GB::OP91() { SUB(GetByteRegister(C_REGISTER)); }; // SUB C
void GB::OP92() { SUB(GetByteRegister(D_REGISTER)); }; // SUB D
void GB::OP93() { SUB(GetByteRegister(E_REGISTER)); }; // SUB E
void GB::OP94() { SUB(GetByteRegister(H_REGISTER)); }; // SUB H
void GB::OP95() { SUB(GetByteRegister(L_REGISTER)); }; // SUB L
void GB::OP96() { SUB(ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // SUB HL
void GB::OP97() { SUB(GetByteRegister(A_REGISTER)); }; // SUB A
void GB::OP98() { SBC(GetByteRegister(B_REGISTER)); }; // SBC A, B
void GB::OP99() { SBC(GetByteRegister(C_REGISTER));  }; // SBC A, C
void GB::OP9A() { SBC(GetByteRegister(D_REGISTER));  }; // SBC A, D
void GB::OP9B() { SBC(GetByteRegister(E_REGISTER));  }; // SBC A, E
void GB::OP9C() { SBC(GetByteRegister(H_REGISTER));  }; // SBC A, H
void GB::OP9D() { SBC(GetByteRegister(L_REGISTER));  }; // SBC A, L
void GB::OP9E() { SBC(ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // SBC A, (HL)
void GB::OP9F() { SBC(GetByteRegister(A_REGISTER));  }; // SBC A, A
void GB::OPA0() { AND(GetByteRegister(B_REGISTER)); }; // AND A, B
void GB::OPA1() { AND(GetByteRegister(C_REGISTER)); }; // AND A, C
void GB::OPA2() { AND(GetByteRegister(D_REGISTER)); }; // AND A, D
void GB::OPA3() { AND(GetByteRegister(E_REGISTER)); }; // AND A, E
void GB::OPA4() { AND(GetByteRegister(H_REGISTER)); }; // AND A, H
void GB::OPA5() { AND(GetByteRegister(L_REGISTER)); }; // AND A, L
void GB::OPA6() { AND(ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // AND A, (HL)
void GB::OPA7() { AND(GetByteRegister(A_REGISTER)); }; // AND A, A
void GB::OPA8() { XOR(GetByteRegister(B_REGISTER)); }; // XOR A, B
void GB::OPA9() { XOR(GetByteRegister(C_REGISTER)); }; // XOR A, C
void GB::OPAA() { XOR(GetByteRegister(D_REGISTER)); }; // XOR A, D
void GB::OPAB() { XOR(GetByteRegister(E_REGISTER)); }; // XOR A, E
void GB::OPAC() { XOR(GetByteRegister(H_REGISTER)); }; // XOR A, H
void GB::OPAD() { XOR(GetByteRegister(L_REGISTER)); }; // XOR A, L
void GB::OPAE() { XOR(ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // XOR A, (HL)
void GB::OPAF() { XOR(GetByteRegister(A_REGISTER)); }; // XOR A, A
void GB::OPB0() { OR(GetByteRegister(B_REGISTER)); }; // OR B, B
void GB::OPB1() { OR(GetByteRegister(C_REGISTER)); }; // OR A, C
void GB::OPB2() { OR(GetByteRegister(D_REGISTER)); }; // OR B, D
void GB::OPB3() { OR(GetByteRegister(E_REGISTER)); }; // OR B, E
void GB::OPB4() { OR(GetByteRegister(H_REGISTER)); }; // OR B, H
void GB::OPB5() { OR(GetByteRegister(L_REGISTER)); }; // OR B, L
void GB::OPB6() { OR(ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // OR B, (HL)
void GB::OPB7() { OR(GetByteRegister(A_REGISTER)); }; // OR B, A
void GB::OPB8() { CP(GetByteRegister(B_REGISTER)); }; // CP A, B
void GB::OPB9() { CP(GetByteRegister(C_REGISTER)); }; // CP A, C
void GB::OPBA() { CP(GetByteRegister(D_REGISTER)); }; // CP A, D
void GB::OPBB() { CP(GetByteRegister(E_REGISTER)); }; // CP A, E
void GB::OPBC() { CP(GetByteRegister(H_REGISTER)); }; // CP A, H
void GB::OPBD() { CP(GetByteRegister(L_REGISTER)); }; // CP A, L
void GB::OPBE() { CP(ReadData(GetWordRegister(HL_REGISTER))); cycle += 4; }; // CP A, (HL)
void GB::OPBF() { CP(GetByteRegister(A_REGISTER)); }; // CP A, A
void GB::OPC0() 
{
	if (!CheckFlag(FLAG_ZERO))
	{
		PopStackPC();
	}
	cycle += 4;
}; // RET NZ
void GB::OPC1() { PopStack(BC_REGISTER); }; // POP BC
void GB::OPC2() 
{
	ui16 address = ReadWord();
	if (!CheckFlag(FLAG_ZERO))
	{
		Jr(address);
	}
}; // JP NZ, u16
void GB::OPC3() { SetPC(ReadWord()); }; //JP u16
void GB::OPC4() 
{
	ui16 word = ReadWord();
	if (!CheckFlag(FLAG_ZERO))
	{
		PushStack(PC_REGISTER);
		SetPC(word);
	}
};
void GB::OPC5() { PushStack(BC_REGISTER); cycle += 4; }; // PUSH BC
void GB::OPC6() { ADD(A_REGISTER, ReadByte()); }; // ADD A, ui8
void GB::OPC7() {assert("Missing" && 0);};
void GB::OPC8() 
{
	if (CheckFlag(FLAG_ZERO))
	{
		PopStackPC();
	}
	cycle += 4;
}; // RET Z
void GB::OPC9() { PopStackPC(); }; // RET
void GB::OPCA() 
{
	ui16 address = ReadWord();
	if (CheckFlag(FLAG_ZERO))
	{
		Jr(address);
	}
}; // JP Z, u16
void GB::OPCB() {assert("Missing" && 0);};
void GB::OPCC() 
{
	ui16 word = ReadWord();
	if (CheckFlag(FLAG_ZERO))
	{
		PushStack(PC_REGISTER);
		SetPC(word);
	}
}; // CALL Z, u16
void GB::OPCD()
{
	ui16 word = ReadWord(); //Get the address 
	PushStack(PC_REGISTER); // Stores PC onto SP
	SetPC(word); // Move to the address};
} // CALL u16 (Push address of next instruction onto stack and then jump to address)
void GB::OPCE() { ADC(ReadByte()); }; // ADC A, ui8
void GB::OPCF() 
{
	PushStack(PC_REGISTER);
	SetPC(RESET_08);
};
void GB::OPD0() 
{
	if (!CheckFlag(FLAG_CARRY))
	{
		PopStackPC();
	}
	cycle += 4;
};
void GB::OPD1() { PopStack(DE_REGISTER); }; // POP DE
void GB::OPD2() {assert("Missing" && 0);};
void GB::OPD3() {assert("Missing" && 0);};
void GB::OPD4() 
{
	ui16 word = ReadWord();
	if (!CheckFlag(FLAG_CARRY))
	{
		PushStack(PC_REGISTER);
		SetPC(word);
	}
}; // CALL NC, ui16
void GB::OPD5() { PushStack(DE_REGISTER); cycle += 4; }; // PUSH DE
void GB::OPD6() { SUB(ReadByte()); }; // SUB ui8
void GB::OPD7() 
{
	PushStack(PC_REGISTER);
	SetPC(RESET_10);
}; // RST 10
void GB::OPD8() 
{
	if (CheckFlag(FLAG_CARRY))
	{
		PopStackPC();
	}
	cycle += 4;
};
void GB::OPD9() 
{
	PopStackPC();
	interruptsEnabled = true;
};
void GB::OPDA() {assert("Missing" && 0);};
void GB::OPDB() {assert("Missing" && 0);};
void GB::OPDC() 
{
	ui16 word = ReadWord();
	if (CheckFlag(FLAG_CARRY))
	{
		PushStack(PC_REGISTER);
		SetPC(word);
	}
}; // CALL C, u16
void GB::OPDD() {assert("Invalid CPU Instruction" && 0);};
void GB::OPDE() { SBC(ReadByte()); }; // SBC A, u8
void GB::OPDF() 
{
	PushStack(PC_REGISTER);
	SetPC(RESET_18);
}; //RST 18
void GB::OPE0() { WriteData(static_cast<ui16> (0xFF00 + ReadByte()), GetByteRegister(A_REGISTER)); cycle += 4; }; // LD (FF00+UI8), A
void GB::OPE1() { PopStack(HL_REGISTER); }; // POP HL
void GB::OPE2() { WriteData(static_cast<ui16> (0xFF00 + GetByteRegister(C_REGISTER)), GetByteRegister(A_REGISTER)); cycle += 4; }; // LD (FF00 + C), A
void GB::OPE3() {assert("Missing" && 0);};
void GB::OPE4() {assert("Missing" && 0);};
void GB::OPE5() { PushStack(HL_REGISTER); cycle += 4; }; // PUSH HL
void GB::OPE6() { AND(ReadByte()); }; // AND A, ui8
void GB::OPE7() {assert("Missing" && 0);};
void GB::OPE8() 
{
	i8 value = static_cast<i8>(*dynamicPtr);

	int result = GetWordRegister(SP_REGISTER) + value;

	ClearFlags();
	if (((GetWordRegister(SP_REGISTER) ^ value ^ (result & 0xFFFF)) & 0x100) == 0x100)
	{
		SetFlag(FLAG_CARRY, true);
	}
	if (((GetWordRegister(SP_REGISTER) ^ value ^ (result & 0xFFFF)) & 0x10) == 0x10)
	{
		SetFlag(FLAG_HALFCARRY, true);
	}

	SetWordRegister(SP_REGISTER, static_cast<ui16>(result));
	IncrementPC();
	//TODO: doesn't inc pc on gbc?
}; // ADD SP, i8
void GB::OPE9() { SetPC(GetWordRegister(HL_REGISTER)); }; // JP HL
void GB::OPEA() { WriteData(ReadWord(), GetByteRegister(A_REGISTER)); cycle += 4; };  //LD (FF00+u8), A
void GB::OPEB() {assert("Invalid CPU Instruction" && 0);};
void GB::OPEC() {assert("Invalid CPU Instruction" && 0);};
void GB::OPED() {assert("Invalid CPU Instruction" && 0);};
void GB::OPEE() { XOR(ReadByte()); }; // XOR A, ui8
void GB::OPEF() 
{
	PushStack(PC_REGISTER);
	SetPC(RESET_28);
};
void GB::OPF0() { SetByteRegister(A_REGISTER, ReadData(static_cast<ui16>(0xFF00 + ReadByte()))); cycle += 4; }; // LD A, (FF00+u8)
void GB::OPF1() 
{
	PopStack(AF_REGISTER);
	GetByteRegister(F_REGISTER) &= 0xF0;
}; // POP AF
void GB::OPF2() { SetByteRegister(A_REGISTER, ReadData(static_cast<ui16> (0xFF00 + GetByteRegister(C_REGISTER)))); cycle += 4; };
void GB::OPF3() { interruptsEnabled = false; }; // DI
void GB::OPF4() {assert("Invalid CPU Instruction" && 0);};
void GB::OPF5() { PushStack(AF_REGISTER); cycle += 4; }; // PUSH AF
void GB::OPF6() { OR(ReadByte()); }; //OR ui8
void GB::OPF7() 
{
	PushStack(PC_REGISTER);
	SetPC(RESET_30);
}; // RST 30h
void GB::OPF8() 
{
	i8 n = static_cast<i8>(*dynamicPtr);
	//i8 n = ReadSignedByteFromPC();
	ui16 result = GetWordRegister(SP_REGISTER) + n;

	ClearFlags();
	if (((GetWordRegister(SP_REGISTER) ^ n ^ result) & 0x100) == 0x100)
	{
		SetFlag(FLAG_CARRY, true);
	}
	if ((((GetWordRegister(SP_REGISTER) ^ n ^ result) & 0x10) == 0x10))
	{
		SetFlag(FLAG_HALFCARRY, true);
	}

	SetWordRegister(HL_REGISTER, result);
	IncrementPC();

	//TODO: again why incpc
}; // LD HL, SP+i8
void GB::OPF9() { SetWordRegister(SP_REGISTER, GetWordRegister(HL_REGISTER)); cycle += 4; };
void GB::OPFA() { SetByteRegister(A_REGISTER, ReadData(ReadWord())); cycle += 4; }; // LD A, (u16)
void GB::OPFB()
{
	interruptsEnabled = true; //Enables interrupts (presumably mode 1 on the z80?) http://jgmalcolm.com/z80/advanced/im1i
} // EI
void GB::OPFC() { assert(0 && "Invalid CPU"); };
void GB::OPFD() { assert(0 && "Invalid CPU"); };
void GB::OPFE() { CP(ReadByte()); }; // CP, n
void GB::OPFF() 
{
	PushStack(PC_REGISTER);
	SetPC(RESET_38);
}; // RST 38h 

//CB OP CODES
void GB::OPCB00() { RLC(GetByteRegister(B_REGISTER), false); }; // RLC B
void GB::OPCB01() { RLC(GetByteRegister(C_REGISTER), false); }; // RLC C
void GB::OPCB02() { RLC(GetByteRegister(D_REGISTER), false); }; // RLC D
void GB::OPCB03() { RLC(GetByteRegister(E_REGISTER), false); }; // RLC E
void GB::OPCB04() { RLC(GetByteRegister(H_REGISTER), false); }; // RLC H
void GB::OPCB05() { RLC(GetByteRegister(L_REGISTER), false); }; // RLC L
void GB::OPCB06() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	RLC(data, false);
	WriteData(GetWordRegister(HL_REGISTER), data);
}; //RLC (HL)
void GB::OPCB07() { RLC(GetByteRegister(A_REGISTER), false); }; // RLC, A
void GB::OPCB08() { RRC(GetByteRegister(B_REGISTER), false); }; // RRC B
void GB::OPCB09() { RRC(GetByteRegister(C_REGISTER), false); }; // RRC C
void GB::OPCB0A() { RRC(GetByteRegister(D_REGISTER), false); }; // RRC D
void GB::OPCB0B() { RRC(GetByteRegister(E_REGISTER), false); }; // RRC E
void GB::OPCB0C() { RRC(GetByteRegister(H_REGISTER), false); }; // RRC H
void GB::OPCB0D() { RRC(GetByteRegister(L_REGISTER), false); }; // RRC L
void GB::OPCB0E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	RRC(data, false);
	WriteData(GetWordRegister(HL_REGISTER), data);
}; // RRC (HL)
void GB::OPCB0F() { RRC(GetByteRegister(A_REGISTER), false); }; // RRC A
void GB::OPCB10() { RL(GetByteRegister(B_REGISTER), false); }; // RL, B
void GB::OPCB11() { RL(GetByteRegister(C_REGISTER), false); }; // RL, C
void GB::OPCB12() { RL(GetByteRegister(D_REGISTER), false); };	// RL, D
void GB::OPCB13() { RL(GetByteRegister(E_REGISTER), false); };	// RL, E
void GB::OPCB14() { RL(GetByteRegister(H_REGISTER), false); };	// RL, H
void GB::OPCB15() { RL(GetByteRegister(L_REGISTER), false); };	// RL, L
void GB::OPCB16() { RL(ReadData(GetWordRegister(HL_REGISTER)), false);  cycle += 8; }; // RL, (HL)
void GB::OPCB17() { RL(GetByteRegister(A_REGISTER), false); }; // RL, A
void GB::OPCB18() { RR(GetByteRegister(B_REGISTER), false); }; // RR B
void GB::OPCB19() { RR(GetByteRegister(C_REGISTER), false); }; // RR C
void GB::OPCB1A() { RR(GetByteRegister(D_REGISTER), false); }; // RR D
void GB::OPCB1B() { RR(GetByteRegister(E_REGISTER), false); }; // RR E
void GB::OPCB1C() { RR(GetByteRegister(H_REGISTER), false); }; // RR H
void GB::OPCB1D() { RR(GetByteRegister(L_REGISTER), false); }; // RR L
void GB::OPCB1E() { RR(ReadData(GetWordRegister(HL_REGISTER)), false); cycle += 8; }; // RR (HL)
void GB::OPCB1F() { RR(GetByteRegister(A_REGISTER), false); }; // RR A
void GB::OPCB20() { SLA(GetByteRegister(B_REGISTER)); }; // SLA, B
void GB::OPCB21() { SLA(GetByteRegister(C_REGISTER)); }; // SLA, C
void GB::OPCB22() { SLA(GetByteRegister(D_REGISTER)); }; // SLA, D
void GB::OPCB23() { SLA(GetByteRegister(E_REGISTER)); }; // SLA, E
void GB::OPCB24() { SLA(GetByteRegister(H_REGISTER)); }; // SLA, H
void GB::OPCB25() { SLA(GetByteRegister(L_REGISTER)); }; // SLA, L
void GB::OPCB26() { SLA(ReadData(GetWordRegister(HL_REGISTER))); cycle += 8; }; // SLA, (HL)
void GB::OPCB27() { SLA(GetByteRegister(A_REGISTER)); }; // SLA, A
void GB::OPCB28() { SRA(GetByteRegister(B_REGISTER)); }; // SRA, B
void GB::OPCB29() { SRA(GetByteRegister(C_REGISTER)); }; // SRA, C
void GB::OPCB2A() { SRA(GetByteRegister(D_REGISTER)); }; // SRA, D
void GB::OPCB2B() { SRA(GetByteRegister(E_REGISTER)); }; // SRA, E
void GB::OPCB2C() { SRA(GetByteRegister(H_REGISTER)); }; // SRA, H
void GB::OPCB2D() { SRA(GetByteRegister(L_REGISTER)); }; // SRA, L
void GB::OPCB2E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SRA(data);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
}; // SRA, (HL)
void GB::OPCB2F() { SRA(GetByteRegister(A_REGISTER)); }; // SRA, A
void GB::OPCB30() { Swap(GetByteRegister(B_REGISTER)); }; // SWAP, B
void GB::OPCB31() { Swap(GetByteRegister(C_REGISTER)); }; // SWAP, C
void GB::OPCB32() { Swap(GetByteRegister(D_REGISTER)); }; // SWAP, D
void GB::OPCB33() { Swap(GetByteRegister(E_REGISTER)); }; // SWAP, E
void GB::OPCB34() { Swap(GetByteRegister(H_REGISTER)); }; // SWAP, H
void GB::OPCB35() { Swap(GetByteRegister(L_REGISTER)); }; // SWAP, L
void GB::OPCB36() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Swap(data);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
}; //SWAP (HL)
void GB::OPCB37() { Swap(GetByteRegister(A_REGISTER)); }; // SWAP, A
void GB::OPCB38() { SRL(GetByteRegister(B_REGISTER)); }; // SRL, B
void GB::OPCB39() { SRL(GetByteRegister(C_REGISTER)); }; // SRL, C
void GB::OPCB3A() { SRL(GetByteRegister(D_REGISTER)); }; // SRL, D
void GB::OPCB3B() { SRL(GetByteRegister(E_REGISTER)); }; // SRL, E
void GB::OPCB3C() { SRL(GetByteRegister(H_REGISTER)); }; // SRL, H
void GB::OPCB3D() { SRL(GetByteRegister(L_REGISTER)); }; // SRL, L
void GB::OPCB3E() { SRL(ReadData(GetWordRegister(HL_REGISTER))); cycle += 8; }; // SRL, (HL)
void GB::OPCB3F() { SRL(GetByteRegister(A_REGISTER)); }; // SRL, A
void GB::OPCB40() { Bit(GetByteRegister(B_REGISTER), 0); }; // BIT 0, B
void GB::OPCB41() { Bit(GetByteRegister(C_REGISTER), 0); };	// BIT 0, C
void GB::OPCB42() { Bit(GetByteRegister(D_REGISTER), 0); };	// BIT 0, D
void GB::OPCB43() { Bit(GetByteRegister(E_REGISTER), 0); };	// BIT 0, E
void GB::OPCB44() { Bit(GetByteRegister(H_REGISTER), 0); };	// BIT 0, H
void GB::OPCB45() { Bit(GetByteRegister(L_REGISTER), 0); };	// BIT 0, L
void GB::OPCB46() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 0);
	cycle += 4;
}; // BIT 0, (HL)
void GB::OPCB47() { Bit(GetByteRegister(A_REGISTER), 0); }; // BIT 0, A
void GB::OPCB48() { Bit(GetByteRegister(B_REGISTER), 1); };	// BIT 1, B
void GB::OPCB49() { Bit(GetByteRegister(C_REGISTER), 1); };	// BIT 1, C
void GB::OPCB4A() { Bit(GetByteRegister(D_REGISTER), 1); };	// BIT 1, D
void GB::OPCB4B() { Bit(GetByteRegister(E_REGISTER), 1); };	// BIT 1, E
void GB::OPCB4C() { Bit(GetByteRegister(H_REGISTER), 1); };	// BIT 1, H
void GB::OPCB4D() { Bit(GetByteRegister(L_REGISTER), 1); };	// BIT 1, L
void GB::OPCB4E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 1);
	cycle += 4;
};
void GB::OPCB4F() { Bit(GetByteRegister(A_REGISTER), 1); }; // BIT 1, A
void GB::OPCB50() { Bit(GetByteRegister(B_REGISTER), 2); }; // BIT B, 2
void GB::OPCB51() { Bit(GetByteRegister(C_REGISTER), 2); }; // BIT C, 2
void GB::OPCB52() { Bit(GetByteRegister(D_REGISTER), 2); }; // BIT D, 2
void GB::OPCB53() { Bit(GetByteRegister(E_REGISTER), 2); }; // BIT E, 2
void GB::OPCB54() { Bit(GetByteRegister(H_REGISTER), 2); }; // BIT H, 2
void GB::OPCB55() { Bit(GetByteRegister(L_REGISTER), 2); }; // BIT L, 2
void GB::OPCB56() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 2); cycle += 4;
};
void GB::OPCB57() { Bit(GetByteRegister(A_REGISTER), 2); }; // BIT A, 2
void GB::OPCB58() { Bit(GetByteRegister(B_REGISTER), 3); }; // BIT B, 3
void GB::OPCB59() { Bit(GetByteRegister(C_REGISTER), 3); }; // BIT C, 3
void GB::OPCB5A() { Bit(GetByteRegister(D_REGISTER), 3); }; // BIT D, 3
void GB::OPCB5B() { Bit(GetByteRegister(E_REGISTER), 3); }; // BIT E, 3
void GB::OPCB5C() { Bit(GetByteRegister(H_REGISTER), 3); }; // BIT H, 3
void GB::OPCB5D() { Bit(GetByteRegister(L_REGISTER), 3); }; // BIT L, 3
void GB::OPCB5E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 3); cycle += 4;
}; // BIT (HL), 3
void GB::OPCB5F() { Bit(GetByteRegister(A_REGISTER), 3); }; // BIT A, 3
void GB::OPCB60() { Bit(GetByteRegister(B_REGISTER), 3); }; // BIT B, 4
void GB::OPCB61() { Bit(GetByteRegister(C_REGISTER), 3); }; // BIT C, 4
void GB::OPCB62() { Bit(GetByteRegister(D_REGISTER), 3); }; // BIT D, 4
void GB::OPCB63() { Bit(GetByteRegister(E_REGISTER), 3); }; // BIT E, 4
void GB::OPCB64() { Bit(GetByteRegister(H_REGISTER), 3); }; // BIT H, 4
void GB::OPCB65() { Bit(GetByteRegister(L_REGISTER), 3); }; // BIT L, 4
void GB::OPCB66() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 4);
}; // BIT (HL), 4
void GB::OPCB67() { Bit(GetByteRegister(A_REGISTER), 4); }; // BIT A, 4
void GB::OPCB68() { Bit(GetByteRegister(B_REGISTER), 5); };	// BIT B, 5
void GB::OPCB69() { Bit(GetByteRegister(C_REGISTER), 5); };	// BIT C, 5
void GB::OPCB6A() { Bit(GetByteRegister(D_REGISTER), 5); };	// BIT D, 5
void GB::OPCB6B() { Bit(GetByteRegister(E_REGISTER), 5); };	// BIT E, 5
void GB::OPCB6C() { Bit(GetByteRegister(H_REGISTER), 5); };	// BIT H, 5
void GB::OPCB6D() { Bit(GetByteRegister(L_REGISTER), 5); };	// BIT L, 5
void GB::OPCB6E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 5); cycle += 4; 
}; // BIT (HL), 5
void GB::OPCB6F() { Bit(GetByteRegister(A_REGISTER), 5); }; // BIT A, 5
void GB::OPCB70() { Bit(GetByteRegister(B_REGISTER), 6); }; // BIT B, 6
void GB::OPCB71() { Bit(GetByteRegister(C_REGISTER), 6); }; // BIT C, 6
void GB::OPCB72() { Bit(GetByteRegister(D_REGISTER), 6); }; // BIT D, 6
void GB::OPCB73() { Bit(GetByteRegister(E_REGISTER), 6); }; // BIT E, 6
void GB::OPCB74() { Bit(GetByteRegister(H_REGISTER), 6); }; // BIT H, 6
void GB::OPCB75() { Bit(GetByteRegister(L_REGISTER), 6); }; // BIT L, 6
void GB::OPCB76() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 6);
	cycle += 4;
}; // BIT, (HL), 6
void GB::OPCB77() { Bit(GetByteRegister(A_REGISTER), 6); }; // BIT A, 6
void GB::OPCB78() { Bit(GetByteRegister(B_REGISTER), 7); }; // BIT B, 7
void GB::OPCB79() { Bit(GetByteRegister(C_REGISTER), 7); };	// BIT C, 7
void GB::OPCB7A() { Bit(GetByteRegister(D_REGISTER), 7); };	// BIT D, 7
void GB::OPCB7B() { Bit(GetByteRegister(E_REGISTER), 7); };	// BIT E, 7
void GB::OPCB7C() { Bit(GetByteRegister(H_REGISTER), 7); };	// BIT H, 7
void GB::OPCB7D() { Bit(GetByteRegister(L_REGISTER), 7); };	// BIT L, 7
void GB::OPCB7E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	Bit(data, 7);
}; //BIT (HL), 7
void GB::OPCB7F() { Bit(GetByteRegister(A_REGISTER), 7); }; // BIT A, 7
void GB::OPCB80() { ClearBit(GetByteRegister(B_REGISTER), 0) ;}; // RES B, 0
void GB::OPCB81() { ClearBit(GetByteRegister(C_REGISTER), 0); }; // RES C, 0
void GB::OPCB82() { ClearBit(GetByteRegister(D_REGISTER), 0); }; // RES D, 0
void GB::OPCB83() { ClearBit(GetByteRegister(E_REGISTER), 0); }; // RES E, 0
void GB::OPCB84() { ClearBit(GetByteRegister(H_REGISTER), 0); }; // RES H, 0
void GB::OPCB85() { ClearBit(GetByteRegister(L_REGISTER), 0); }; // RES L, 0
void GB::OPCB86() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 0);
	WriteData(GetWordRegister(HL_REGISTER), data); cycle += 8; 
}; // BIT HL, 7
void GB::OPCB87() { ClearBit(GetByteRegister(A_REGISTER), 0); }; // RES A, 0
void GB::OPCB88() { ClearBit(GetByteRegister(B_REGISTER), 1); }; // RES B, 1
void GB::OPCB89() { ClearBit(GetByteRegister(C_REGISTER), 1); }; // RES C, 1
void GB::OPCB8A() { ClearBit(GetByteRegister(D_REGISTER), 1); }; // RES D, 1
void GB::OPCB8B() { ClearBit(GetByteRegister(E_REGISTER), 1); }; // RES E, 1
void GB::OPCB8C() { ClearBit(GetByteRegister(H_REGISTER), 1); }; // RES H, 1
void GB::OPCB8D() { ClearBit(GetByteRegister(L_REGISTER), 1); }; // RES L, 1
void GB::OPCB8E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 1);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8; 
};
void GB::OPCB8F() { ClearBit(GetByteRegister(A_REGISTER), 1); }; //RES A, 1
void GB::OPCB90() { ClearBit(GetByteRegister(B_REGISTER), 2); }; //RES B, 2
void GB::OPCB91() { ClearBit(GetByteRegister(C_REGISTER), 2); }; //RES C, 2
void GB::OPCB92() { ClearBit(GetByteRegister(D_REGISTER), 2); }; //RES D, 2
void GB::OPCB93() { ClearBit(GetByteRegister(E_REGISTER), 2); }; //RES E, 2
void GB::OPCB94() { ClearBit(GetByteRegister(H_REGISTER), 2); }; //RES H, 2
void GB::OPCB95() { ClearBit(GetByteRegister(L_REGISTER), 2); }; //RES L, 2
void GB::OPCB96() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 2);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8; 
};
void GB::OPCB97() { ClearBit(GetByteRegister(A_REGISTER), 2); }; //RES A, 2
void GB::OPCB98() { ClearBit(GetByteRegister(B_REGISTER), 3); }; //RES B, 3
void GB::OPCB99() { ClearBit(GetByteRegister(C_REGISTER), 3); }; //RES C, 3
void GB::OPCB9A() { ClearBit(GetByteRegister(D_REGISTER), 3); }; //RES D, 3
void GB::OPCB9B() { ClearBit(GetByteRegister(E_REGISTER), 3); }; //RES E, 3
void GB::OPCB9C() { ClearBit(GetByteRegister(H_REGISTER), 3); }; //RES H, 3
void GB::OPCB9D() { ClearBit(GetByteRegister(L_REGISTER), 3); }; //RES L, 3
void GB::OPCB9E() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 3);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCB9F() { ClearBit(GetByteRegister(A_REGISTER), 3); }; //RES A, 3
void GB::OPCBA0() { ClearBit(GetByteRegister(B_REGISTER), 4); }; //RES B, 4
void GB::OPCBA1() { ClearBit(GetByteRegister(C_REGISTER), 4); }; //RES C, 4
void GB::OPCBA2() { ClearBit(GetByteRegister(D_REGISTER), 4); }; //RES D, 4
void GB::OPCBA3() { ClearBit(GetByteRegister(E_REGISTER), 4); }; //RES E, 4
void GB::OPCBA4() { ClearBit(GetByteRegister(H_REGISTER), 4); }; //RES H, 4
void GB::OPCBA5() { ClearBit(GetByteRegister(L_REGISTER), 4); }; //RES L, 4
void GB::OPCBA6() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 4);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBA7() { ClearBit(GetByteRegister(A_REGISTER), 4); }; //RES A, 4
void GB::OPCBA8() { ClearBit(GetByteRegister(B_REGISTER), 5); }; //RES B, 5
void GB::OPCBA9() { ClearBit(GetByteRegister(C_REGISTER), 5); }; //RES C, 5
void GB::OPCBAA() { ClearBit(GetByteRegister(D_REGISTER), 5); }; //RES D, 5
void GB::OPCBAB() { ClearBit(GetByteRegister(E_REGISTER), 5); }; //RES E, 5
void GB::OPCBAC() { ClearBit(GetByteRegister(H_REGISTER), 5); }; //RES H, 5
void GB::OPCBAD() { ClearBit(GetByteRegister(L_REGISTER), 5); }; //RES L, 5
void GB::OPCBAE() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 5);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBAF() { ClearBit(GetByteRegister(A_REGISTER), 5); }; //RES A, 5
void GB::OPCBB0() { ClearBit(GetByteRegister(B_REGISTER), 6); }; //RES B, 6
void GB::OPCBB1() { ClearBit(GetByteRegister(C_REGISTER), 6); }; //RES C, 6
void GB::OPCBB2() { ClearBit(GetByteRegister(D_REGISTER), 6); }; //RES D, 6
void GB::OPCBB3() { ClearBit(GetByteRegister(E_REGISTER), 6); }; //RES E, 6
void GB::OPCBB4() { ClearBit(GetByteRegister(H_REGISTER), 6); }; //RES H, 6
void GB::OPCBB5() { ClearBit(GetByteRegister(L_REGISTER), 6); }; //RES L, 6
void GB::OPCBB6() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 6);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBB7() { ClearBit(GetByteRegister(A_REGISTER), 6); }; //RES A, 6
void GB::OPCBB8() { ClearBit(GetByteRegister(B_REGISTER), 7); }; //RES B, 7
void GB::OPCBB9() { ClearBit(GetByteRegister(C_REGISTER), 7); }; //RES C, 7
void GB::OPCBBA() { ClearBit(GetByteRegister(D_REGISTER), 7); }; //RES D, 7
void GB::OPCBBB() { ClearBit(GetByteRegister(E_REGISTER), 7); }; //RES E, 7
void GB::OPCBBC() { ClearBit(GetByteRegister(H_REGISTER), 7); }; //RES H, 7
void GB::OPCBBD() { ClearBit(GetByteRegister(L_REGISTER), 7); }; //RES L, 7
void GB::OPCBBE() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	ClearBit(data, 7);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBBF() { ClearBit(GetByteRegister(L_REGISTER), 7); }; // RES A, 7 
void GB::OPCBC0() { SetBit(GetByteRegister(B_REGISTER), 0); }; // SET 0, B
void GB::OPCBC1() { SetBit(GetByteRegister(C_REGISTER), 0); }; // SET 0, C
void GB::OPCBC2() { SetBit(GetByteRegister(D_REGISTER), 0); }; // SET 0, D
void GB::OPCBC3() { SetBit(GetByteRegister(E_REGISTER), 0); }; // SET 0, E
void GB::OPCBC4() { SetBit(GetByteRegister(H_REGISTER), 0); }; // SET 0, H
void GB::OPCBC5() { SetBit(GetByteRegister(L_REGISTER), 0); }; // SET 0, L
void GB::OPCBC6() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 0);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBC7() { SetBit(GetByteRegister(A_REGISTER), 0); }; // SET 0, A
void GB::OPCBC8() { SetBit(GetByteRegister(B_REGISTER), 1); }; // SET 1, B
void GB::OPCBC9() { SetBit(GetByteRegister(C_REGISTER), 1); }; // SET 1, C
void GB::OPCBCA() { SetBit(GetByteRegister(D_REGISTER), 1); }; // SET 1, D
void GB::OPCBCB() { SetBit(GetByteRegister(E_REGISTER), 1); }; // SET 1, E
void GB::OPCBCC() { SetBit(GetByteRegister(H_REGISTER), 1); }; // SET 1, H
void GB::OPCBCD() { SetBit(GetByteRegister(L_REGISTER), 1); }; // SET 1, L
void GB::OPCBCE() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 1);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBCF() { SetBit(GetByteRegister(A_REGISTER), 1); }; // SET 1, A
void GB::OPCBD0() { SetBit(GetByteRegister(B_REGISTER), 2); }; // SET 2, B
void GB::OPCBD1() { SetBit(GetByteRegister(C_REGISTER), 2); }; // SET 2, C
void GB::OPCBD2() { SetBit(GetByteRegister(D_REGISTER), 2); }; // SET 2, D
void GB::OPCBD3() { SetBit(GetByteRegister(E_REGISTER), 2); }; // SET 2, E
void GB::OPCBD4() { SetBit(GetByteRegister(H_REGISTER), 2); }; // SET 2, H
void GB::OPCBD5() { SetBit(GetByteRegister(L_REGISTER), 2); }; // SET 2, L
void GB::OPCBD6() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 2);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBD7() { SetBit(GetByteRegister(A_REGISTER), 2); }; // SET 2, A
void GB::OPCBD8() { SetBit(GetByteRegister(B_REGISTER), 3); }; // SET 3, B
void GB::OPCBD9() { SetBit(GetByteRegister(C_REGISTER), 3); }; // SET 3, C
void GB::OPCBDA() { SetBit(GetByteRegister(D_REGISTER), 3); }; // SET 3, D
void GB::OPCBDB() { SetBit(GetByteRegister(E_REGISTER), 3); }; // SET 3, E
void GB::OPCBDC() { SetBit(GetByteRegister(H_REGISTER), 3); }; // SET 3, H
void GB::OPCBDD() { SetBit(GetByteRegister(L_REGISTER), 3); }; // SET 3, L
void GB::OPCBDE() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 3);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBDF() { SetBit(GetByteRegister(A_REGISTER), 3); }; // SET 3, A
void GB::OPCBE0() { SetBit(GetByteRegister(B_REGISTER), 4); }; // SET 4, B
void GB::OPCBE1() { SetBit(GetByteRegister(C_REGISTER), 4); }; // SET 4, C
void GB::OPCBE2() { SetBit(GetByteRegister(D_REGISTER), 4); }; // SET 4, D
void GB::OPCBE3() { SetBit(GetByteRegister(E_REGISTER), 4); }; // SET 4, E
void GB::OPCBE4() { SetBit(GetByteRegister(H_REGISTER), 4); }; // SET 4, H
void GB::OPCBE5() { SetBit(GetByteRegister(L_REGISTER), 4); }; // SET 4, L
void GB::OPCBE6() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 4);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBE7() { SetBit(GetByteRegister(A_REGISTER), 4); }; // SET 4, A
void GB::OPCBE8() { SetBit(GetByteRegister(B_REGISTER), 5); }; // SET 5, B
void GB::OPCBE9() { SetBit(GetByteRegister(C_REGISTER), 5); }; // SET 5, C
void GB::OPCBEA() { SetBit(GetByteRegister(D_REGISTER), 5); }; // SET 5, D
void GB::OPCBEB() { SetBit(GetByteRegister(E_REGISTER), 5); }; // SET 5, E
void GB::OPCBEC() { SetBit(GetByteRegister(H_REGISTER), 5); }; // SET 5, H
void GB::OPCBED() { SetBit(GetByteRegister(L_REGISTER), 5); }; // SET 5, L
void GB::OPCBEE() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 5);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBEF() { SetBit(GetByteRegister(A_REGISTER), 5); }; // SET 5, A
void GB::OPCBF0() { SetBit(GetByteRegister(B_REGISTER), 6); }; // SET 6, B
void GB::OPCBF1() { SetBit(GetByteRegister(C_REGISTER), 6); }; // SET 6, C
void GB::OPCBF2() { SetBit(GetByteRegister(D_REGISTER), 6); }; // SET 6, D
void GB::OPCBF3() { SetBit(GetByteRegister(E_REGISTER), 6); }; // SET 6, E
void GB::OPCBF4() { SetBit(GetByteRegister(H_REGISTER), 6); }; // SET 6, H
void GB::OPCBF5() { SetBit(GetByteRegister(L_REGISTER), 6); }; // SET 6, L
void GB::OPCBF6() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 6);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBF7() { SetBit(GetByteRegister(A_REGISTER), 6); }; // SET 6, A
void GB::OPCBF8() { SetBit(GetByteRegister(B_REGISTER), 7); }; // SET 7, B
void GB::OPCBF9() { SetBit(GetByteRegister(C_REGISTER), 7); }; // SET 7, C
void GB::OPCBFA() { SetBit(GetByteRegister(D_REGISTER), 7); }; // SET 7, D
void GB::OPCBFB() { SetBit(GetByteRegister(E_REGISTER), 7); }; // SET 7, E
void GB::OPCBFC() { SetBit(GetByteRegister(H_REGISTER), 7); }; // SET 7, H
void GB::OPCBFD() { SetBit(GetByteRegister(L_REGISTER), 7); }; // SET 7, L
void GB::OPCBFE() 
{
	ui8& data = ReadData(GetWordRegister(HL_REGISTER));
	SetBit(data, 6);
	WriteData(GetWordRegister(HL_REGISTER), data);
	cycle += 8;
};
void GB::OPCBFF() { SetBit(GetByteRegister(A_REGISTER), 7); }; // SET 7, A


//Stores all of the CPU Instructions into a single tidy array
void GB::InitOPArray()
{
	BASECodes[0x00] = &GB::OP00;
	BASECodes[0x01] = &GB::OP01;
	BASECodes[0x02] = &GB::OP02;
	BASECodes[0x03] = &GB::OP03;
	BASECodes[0x04] = &GB::OP04;
	BASECodes[0x05] = &GB::OP05;
	BASECodes[0x06] = &GB::OP06;
	BASECodes[0x07] = &GB::OP07;
	BASECodes[0x08] = &GB::OP08;
	BASECodes[0x09] = &GB::OP09;
	BASECodes[0x0A] = &GB::OP0A;
	BASECodes[0x0B] = &GB::OP0B;
	BASECodes[0x0C] = &GB::OP0C;
	BASECodes[0x0D] = &GB::OP0D;
	BASECodes[0x0E] = &GB::OP0E;
	BASECodes[0x0F] = &GB::OP0F;
	BASECodes[0x10] = &GB::OP10;
	BASECodes[0x11] = &GB::OP11;
	BASECodes[0x12] = &GB::OP12;
	BASECodes[0x13] = &GB::OP13;
	BASECodes[0x14] = &GB::OP14;
	BASECodes[0x15] = &GB::OP15;
	BASECodes[0x16] = &GB::OP16;
	BASECodes[0x17] = &GB::OP17;
	BASECodes[0x18] = &GB::OP18;
	BASECodes[0x19] = &GB::OP19;
	BASECodes[0x1A] = &GB::OP1A;
	BASECodes[0x1B] = &GB::OP1B;
	BASECodes[0x1C] = &GB::OP1C;
	BASECodes[0x1D] = &GB::OP1D;
	BASECodes[0x1E] = &GB::OP1E;
	BASECodes[0x1F] = &GB::OP1F;
	BASECodes[0x20] = &GB::OP20;
	BASECodes[0x21] = &GB::OP21;
	BASECodes[0x22] = &GB::OP22;
	BASECodes[0x23] = &GB::OP23;
	BASECodes[0x24] = &GB::OP24;
	BASECodes[0x25] = &GB::OP25;
	BASECodes[0x26] = &GB::OP26;
	BASECodes[0x27] = &GB::OP27;
	BASECodes[0x28] = &GB::OP28;
	BASECodes[0x29] = &GB::OP29;
	BASECodes[0x2A] = &GB::OP2A;
	BASECodes[0x2B] = &GB::OP2B;
	BASECodes[0x2C] = &GB::OP2C;
	BASECodes[0x2D] = &GB::OP2D;
	BASECodes[0x2E] = &GB::OP2E;
	BASECodes[0x2F] = &GB::OP2F;
	BASECodes[0x30] = &GB::OP30;
	BASECodes[0x31] = &GB::OP31;
	BASECodes[0x32] = &GB::OP32;
	BASECodes[0x33] = &GB::OP33;
	BASECodes[0x34] = &GB::OP34;
	BASECodes[0x35] = &GB::OP35;
	BASECodes[0x36] = &GB::OP36;
	BASECodes[0x37] = &GB::OP37;
	BASECodes[0x38] = &GB::OP38;
	BASECodes[0x39] = &GB::OP39;
	BASECodes[0x3A] = &GB::OP3A;
	BASECodes[0x3B] = &GB::OP3B;
	BASECodes[0x3C] = &GB::OP3C;
	BASECodes[0x3D] = &GB::OP3D;
	BASECodes[0x3E] = &GB::OP3E;
	BASECodes[0x3F] = &GB::OP3F;
	BASECodes[0x40] = &GB::OP40;
	BASECodes[0x41] = &GB::OP41;
	BASECodes[0x42] = &GB::OP42;
	BASECodes[0x43] = &GB::OP43;
	BASECodes[0x44] = &GB::OP44;
	BASECodes[0x45] = &GB::OP45;
	BASECodes[0x46] = &GB::OP46;
	BASECodes[0x47] = &GB::OP47;
	BASECodes[0x48] = &GB::OP48;
	BASECodes[0x49] = &GB::OP49;
	BASECodes[0x4A] = &GB::OP4A;
	BASECodes[0x4B] = &GB::OP4B;
	BASECodes[0x4C] = &GB::OP4C;
	BASECodes[0x4D] = &GB::OP4D;
	BASECodes[0x4E] = &GB::OP4E;
	BASECodes[0x4F] = &GB::OP4F;
	BASECodes[0x50] = &GB::OP50;
	BASECodes[0x51] = &GB::OP51;
	BASECodes[0x52] = &GB::OP52;
	BASECodes[0x53] = &GB::OP53;
	BASECodes[0x54] = &GB::OP54;
	BASECodes[0x55] = &GB::OP55;
	BASECodes[0x56] = &GB::OP56;
	BASECodes[0x57] = &GB::OP57;
	BASECodes[0x58] = &GB::OP58;
	BASECodes[0x59] = &GB::OP59;
	BASECodes[0x5A] = &GB::OP5A;
	BASECodes[0x5B] = &GB::OP5B;
	BASECodes[0x5C] = &GB::OP5C;
	BASECodes[0x5D] = &GB::OP5D;
	BASECodes[0x5E] = &GB::OP5E;
	BASECodes[0x5F] = &GB::OP5F;
	BASECodes[0x60] = &GB::OP60;
	BASECodes[0x61] = &GB::OP61;
	BASECodes[0x62] = &GB::OP62;
	BASECodes[0x63] = &GB::OP63;
	BASECodes[0x64] = &GB::OP64;
	BASECodes[0x65] = &GB::OP65;
	BASECodes[0x66] = &GB::OP66;
	BASECodes[0x67] = &GB::OP67;
	BASECodes[0x68] = &GB::OP68;
	BASECodes[0x69] = &GB::OP69;
	BASECodes[0x6A] = &GB::OP6A;
	BASECodes[0x6B] = &GB::OP6B;
	BASECodes[0x6C] = &GB::OP6C;
	BASECodes[0x6D] = &GB::OP6D;
	BASECodes[0x6E] = &GB::OP6E;
	BASECodes[0x6F] = &GB::OP6F;
	BASECodes[0x70] = &GB::OP70;
	BASECodes[0x71] = &GB::OP71;
	BASECodes[0x72] = &GB::OP72;
	BASECodes[0x73] = &GB::OP73;
	BASECodes[0x74] = &GB::OP74;
	BASECodes[0x75] = &GB::OP75;
	BASECodes[0x76] = &GB::OP76;
	BASECodes[0x77] = &GB::OP77;
	BASECodes[0x78] = &GB::OP78;
	BASECodes[0x79] = &GB::OP79;
	BASECodes[0x7A] = &GB::OP7A;
	BASECodes[0x7B] = &GB::OP7B;
	BASECodes[0x7C] = &GB::OP7C;
	BASECodes[0x7D] = &GB::OP7D;
	BASECodes[0x7E] = &GB::OP7E;
	BASECodes[0x7F] = &GB::OP7F;
	BASECodes[0x80] = &GB::OP80;
	BASECodes[0x81] = &GB::OP81;
	BASECodes[0x82] = &GB::OP82;
	BASECodes[0x83] = &GB::OP83;
	BASECodes[0x84] = &GB::OP84;
	BASECodes[0x85] = &GB::OP85;
	BASECodes[0x86] = &GB::OP86;
	BASECodes[0x87] = &GB::OP87;
	BASECodes[0x88] = &GB::OP88;
	BASECodes[0x89] = &GB::OP89;
	BASECodes[0x8A] = &GB::OP8A;
	BASECodes[0x8B] = &GB::OP8B;
	BASECodes[0x8C] = &GB::OP8C;
	BASECodes[0x8D] = &GB::OP8D;
	BASECodes[0x8E] = &GB::OP8E;
	BASECodes[0x8F] = &GB::OP8F;
	BASECodes[0x90] = &GB::OP90;
	BASECodes[0x91] = &GB::OP91;
	BASECodes[0x92] = &GB::OP92;
	BASECodes[0x93] = &GB::OP93;
	BASECodes[0x94] = &GB::OP94;
	BASECodes[0x95] = &GB::OP95;
	BASECodes[0x96] = &GB::OP96;
	BASECodes[0x97] = &GB::OP97;
	BASECodes[0x98] = &GB::OP98;
	BASECodes[0x99] = &GB::OP99;
	BASECodes[0x9A] = &GB::OP9A;
	BASECodes[0x9B] = &GB::OP9B;
	BASECodes[0x9C] = &GB::OP9C;
	BASECodes[0x9D] = &GB::OP9D;
	BASECodes[0x9E] = &GB::OP9E;
	BASECodes[0x9F] = &GB::OP9F;
	BASECodes[0xA0] = &GB::OPA0;
	BASECodes[0xA1] = &GB::OPA1;
	BASECodes[0xA2] = &GB::OPA2;
	BASECodes[0xA3] = &GB::OPA3;
	BASECodes[0xA4] = &GB::OPA4;
	BASECodes[0xA5] = &GB::OPA5;
	BASECodes[0xA6] = &GB::OPA6;
	BASECodes[0xA7] = &GB::OPA7;
	BASECodes[0xA8] = &GB::OPA8;
	BASECodes[0xA9] = &GB::OPA9;
	BASECodes[0xAA] = &GB::OPAA;
	BASECodes[0xAB] = &GB::OPAB;
	BASECodes[0xAC] = &GB::OPAC;
	BASECodes[0xAD] = &GB::OPAD;
	BASECodes[0xAE] = &GB::OPAE;
	BASECodes[0xAF] = &GB::OPAF;
	BASECodes[0xB0] = &GB::OPB0;
	BASECodes[0xB1] = &GB::OPB1;
	BASECodes[0xB2] = &GB::OPB2;
	BASECodes[0xB3] = &GB::OPB3;
	BASECodes[0xB4] = &GB::OPB4;
	BASECodes[0xB5] = &GB::OPB5;
	BASECodes[0xB6] = &GB::OPB6;
	BASECodes[0xB7] = &GB::OPB7;
	BASECodes[0xB8] = &GB::OPB8;
	BASECodes[0xB9] = &GB::OPB9;
	BASECodes[0xBA] = &GB::OPBA;
	BASECodes[0xBB] = &GB::OPBB;
	BASECodes[0xBC] = &GB::OPBC;
	BASECodes[0xBD] = &GB::OPBD;
	BASECodes[0xBE] = &GB::OPBE;
	BASECodes[0xBF] = &GB::OPBF;
	BASECodes[0xC0] = &GB::OPC0;
	BASECodes[0xC1] = &GB::OPC1;
	BASECodes[0xC2] = &GB::OPC2;
	BASECodes[0xC3] = &GB::OPC3;
	BASECodes[0xC4] = &GB::OPC4;
	BASECodes[0xC5] = &GB::OPC5;
	BASECodes[0xC6] = &GB::OPC6;
	BASECodes[0xC7] = &GB::OPC7;
	BASECodes[0xC8] = &GB::OPC8;
	BASECodes[0xC9] = &GB::OPC9;
	BASECodes[0xCA] = &GB::OPCA;
	BASECodes[0xCB] = &GB::OPCB;
	BASECodes[0xCC] = &GB::OPCC;
	BASECodes[0xCD] = &GB::OPCD;
	BASECodes[0xCE] = &GB::OPCE;
	BASECodes[0xCF] = &GB::OPCF;
	BASECodes[0xD0] = &GB::OPD0;
	BASECodes[0xD1] = &GB::OPD1;
	BASECodes[0xD2] = &GB::OPD2;
	BASECodes[0xD3] = &GB::OPD3;
	BASECodes[0xD4] = &GB::OPD4;
	BASECodes[0xD5] = &GB::OPD5;
	BASECodes[0xD6] = &GB::OPD6;
	BASECodes[0xD7] = &GB::OPD7;
	BASECodes[0xD8] = &GB::OPD8;
	BASECodes[0xD9] = &GB::OPD9;
	BASECodes[0xDA] = &GB::OPDA;
	BASECodes[0xDB] = &GB::OPDB;
	BASECodes[0xDC] = &GB::OPDC;
	BASECodes[0xDD] = &GB::OPDD;
	BASECodes[0xDE] = &GB::OPDE;
	BASECodes[0xDF] = &GB::OPDF;
	BASECodes[0xE0] = &GB::OPE0;
	BASECodes[0xE1] = &GB::OPE1;
	BASECodes[0xE2] = &GB::OPE2;
	BASECodes[0xE3] = &GB::OPE3;
	BASECodes[0xE4] = &GB::OPE4;
	BASECodes[0xE5] = &GB::OPE5;
	BASECodes[0xE6] = &GB::OPE6;
	BASECodes[0xE7] = &GB::OPE7;
	BASECodes[0xE8] = &GB::OPE8;
	BASECodes[0xE9] = &GB::OPE9;
	BASECodes[0xEA] = &GB::OPEA;
	BASECodes[0xEB] = &GB::OPEB;
	BASECodes[0xEC] = &GB::OPEC;
	BASECodes[0xED] = &GB::OPED;
	BASECodes[0xEE] = &GB::OPEE;
	BASECodes[0xEF] = &GB::OPEF;
	BASECodes[0xF0] = &GB::OPF0;
	BASECodes[0xF1] = &GB::OPF1;
	BASECodes[0xF2] = &GB::OPF2;
	BASECodes[0xF3] = &GB::OPF3;
	BASECodes[0xF4] = &GB::OPF4;
	BASECodes[0xF5] = &GB::OPF5;
	BASECodes[0xF6] = &GB::OPF6;
	BASECodes[0xF7] = &GB::OPF7;
	BASECodes[0xF8] = &GB::OPF8;
	BASECodes[0xF9] = &GB::OPF9;
	BASECodes[0xFA] = &GB::OPFA;
	BASECodes[0xFB] = &GB::OPFB;
	BASECodes[0xFC] = &GB::OPFC;
	BASECodes[0xFD] = &GB::OPFD;
	BASECodes[0xFE] = &GB::OPFE;
	BASECodes[0xFF] = &GB::OPFF;

}

void GB::InitCBOPArray()
{
	CBCodes[0x00] = &GB::OPCB00;
	CBCodes[0x01] = &GB::OPCB01;
	CBCodes[0x02] = &GB::OPCB02;
	CBCodes[0x03] = &GB::OPCB03;
	CBCodes[0x04] = &GB::OPCB04;
	CBCodes[0x05] = &GB::OPCB05;
	CBCodes[0x06] = &GB::OPCB06;
	CBCodes[0x07] = &GB::OPCB07;
	CBCodes[0x08] = &GB::OPCB08;
	CBCodes[0x09] = &GB::OPCB09;
	CBCodes[0x0A] = &GB::OPCB0A;
	CBCodes[0x0B] = &GB::OPCB0B;
	CBCodes[0x0C] = &GB::OPCB0C;
	CBCodes[0x0D] = &GB::OPCB0D;
	CBCodes[0x0E] = &GB::OPCB0E;
	CBCodes[0x0F] = &GB::OPCB0F;
	CBCodes[0x10] = &GB::OPCB10;
	CBCodes[0x11] = &GB::OPCB11;
	CBCodes[0x12] = &GB::OPCB12;
	CBCodes[0x13] = &GB::OPCB13;
	CBCodes[0x14] = &GB::OPCB14;
	CBCodes[0x15] = &GB::OPCB15;
	CBCodes[0x16] = &GB::OPCB16;
	CBCodes[0x17] = &GB::OPCB17;
	CBCodes[0x18] = &GB::OPCB18;
	CBCodes[0x19] = &GB::OPCB19;
	CBCodes[0x1A] = &GB::OPCB1A;
	CBCodes[0x1B] = &GB::OPCB1B;
	CBCodes[0x1C] = &GB::OPCB1C;
	CBCodes[0x1D] = &GB::OPCB1D;
	CBCodes[0x1E] = &GB::OPCB1E;
	CBCodes[0x1F] = &GB::OPCB1F;
	CBCodes[0x20] = &GB::OPCB20;
	CBCodes[0x21] = &GB::OPCB21;
	CBCodes[0x22] = &GB::OPCB22;
	CBCodes[0x23] = &GB::OPCB23;
	CBCodes[0x24] = &GB::OPCB24;
	CBCodes[0x25] = &GB::OPCB25;
	CBCodes[0x26] = &GB::OPCB26;
	CBCodes[0x27] = &GB::OPCB27;
	CBCodes[0x28] = &GB::OPCB28;
	CBCodes[0x29] = &GB::OPCB29;
	CBCodes[0x2A] = &GB::OPCB2A;
	CBCodes[0x2B] = &GB::OPCB2B;
	CBCodes[0x2C] = &GB::OPCB2C;
	CBCodes[0x2D] = &GB::OPCB2D;
	CBCodes[0x2E] = &GB::OPCB2E;
	CBCodes[0x2F] = &GB::OPCB2F;
	CBCodes[0x30] = &GB::OPCB30;
	CBCodes[0x31] = &GB::OPCB31;
	CBCodes[0x32] = &GB::OPCB32;
	CBCodes[0x33] = &GB::OPCB33;
	CBCodes[0x34] = &GB::OPCB34;
	CBCodes[0x35] = &GB::OPCB35;
	CBCodes[0x36] = &GB::OPCB36;
	CBCodes[0x37] = &GB::OPCB37;
	CBCodes[0x38] = &GB::OPCB38;
	CBCodes[0x39] = &GB::OPCB39;
	CBCodes[0x3A] = &GB::OPCB3A;
	CBCodes[0x3B] = &GB::OPCB3B;
	CBCodes[0x3C] = &GB::OPCB3C;
	CBCodes[0x3D] = &GB::OPCB3D;
	CBCodes[0x3E] = &GB::OPCB3E;
	CBCodes[0x3F] = &GB::OPCB3F;
	CBCodes[0x40] = &GB::OPCB40;
	CBCodes[0x41] = &GB::OPCB41;
	CBCodes[0x42] = &GB::OPCB42;
	CBCodes[0x43] = &GB::OPCB43;
	CBCodes[0x44] = &GB::OPCB44;
	CBCodes[0x45] = &GB::OPCB45;
	CBCodes[0x46] = &GB::OPCB46;
	CBCodes[0x47] = &GB::OPCB47;
	CBCodes[0x48] = &GB::OPCB48;
	CBCodes[0x49] = &GB::OPCB49;
	CBCodes[0x4A] = &GB::OPCB4A;
	CBCodes[0x4B] = &GB::OPCB4B;
	CBCodes[0x4C] = &GB::OPCB4C;
	CBCodes[0x4D] = &GB::OPCB4D;
	CBCodes[0x4E] = &GB::OPCB4E;
	CBCodes[0x4F] = &GB::OPCB4F;
	CBCodes[0x50] = &GB::OPCB50;
	CBCodes[0x51] = &GB::OPCB51;
	CBCodes[0x52] = &GB::OPCB52;
	CBCodes[0x53] = &GB::OPCB53;
	CBCodes[0x54] = &GB::OPCB54;
	CBCodes[0x55] = &GB::OPCB55;
	CBCodes[0x56] = &GB::OPCB56;
	CBCodes[0x57] = &GB::OPCB57;
	CBCodes[0x58] = &GB::OPCB58;
	CBCodes[0x59] = &GB::OPCB59;
	CBCodes[0x5A] = &GB::OPCB5A;
	CBCodes[0x5B] = &GB::OPCB5B;
	CBCodes[0x5C] = &GB::OPCB5C;
	CBCodes[0x5D] = &GB::OPCB5D;
	CBCodes[0x5E] = &GB::OPCB5E;
	CBCodes[0x5F] = &GB::OPCB5F;
	CBCodes[0x60] = &GB::OPCB60;
	CBCodes[0x61] = &GB::OPCB61;
	CBCodes[0x62] = &GB::OPCB62;
	CBCodes[0x63] = &GB::OPCB63;
	CBCodes[0x64] = &GB::OPCB64;
	CBCodes[0x65] = &GB::OPCB65;
	CBCodes[0x66] = &GB::OPCB66;
	CBCodes[0x67] = &GB::OPCB67;
	CBCodes[0x68] = &GB::OPCB68;
	CBCodes[0x69] = &GB::OPCB69;
	CBCodes[0x6A] = &GB::OPCB6A;
	CBCodes[0x6B] = &GB::OPCB6B;
	CBCodes[0x6C] = &GB::OPCB6C;
	CBCodes[0x6D] = &GB::OPCB6D;
	CBCodes[0x6E] = &GB::OPCB6E;
	CBCodes[0x6F] = &GB::OPCB6F;
	CBCodes[0x70] = &GB::OPCB70;
	CBCodes[0x71] = &GB::OPCB71;
	CBCodes[0x72] = &GB::OPCB72;
	CBCodes[0x73] = &GB::OPCB73;
	CBCodes[0x74] = &GB::OPCB74;
	CBCodes[0x75] = &GB::OPCB75;
	CBCodes[0x76] = &GB::OPCB76;
	CBCodes[0x77] = &GB::OPCB77;
	CBCodes[0x78] = &GB::OPCB78;
	CBCodes[0x79] = &GB::OPCB79;
	CBCodes[0x7A] = &GB::OPCB7A;
	CBCodes[0x7B] = &GB::OPCB7B;
	CBCodes[0x7C] = &GB::OPCB7C;
	CBCodes[0x7D] = &GB::OPCB7D;
	CBCodes[0x7E] = &GB::OPCB7E;
	CBCodes[0x7F] = &GB::OPCB7F;
	CBCodes[0x80] = &GB::OPCB80;
	CBCodes[0x81] = &GB::OPCB81;
	CBCodes[0x82] = &GB::OPCB82;
	CBCodes[0x83] = &GB::OPCB83;
	CBCodes[0x84] = &GB::OPCB84;
	CBCodes[0x85] = &GB::OPCB85;
	CBCodes[0x86] = &GB::OPCB86;
	CBCodes[0x87] = &GB::OPCB87;
	CBCodes[0x88] = &GB::OPCB88;
	CBCodes[0x89] = &GB::OPCB89;
	CBCodes[0x8A] = &GB::OPCB8A;
	CBCodes[0x8B] = &GB::OPCB8B;
	CBCodes[0x8C] = &GB::OPCB8C;
	CBCodes[0x8D] = &GB::OPCB8D;
	CBCodes[0x8E] = &GB::OPCB8E;
	CBCodes[0x8F] = &GB::OPCB8F;
	CBCodes[0x90] = &GB::OPCB90;
	CBCodes[0x91] = &GB::OPCB91;
	CBCodes[0x92] = &GB::OPCB92;
	CBCodes[0x93] = &GB::OPCB93;
	CBCodes[0x94] = &GB::OPCB94;
	CBCodes[0x95] = &GB::OPCB95;
	CBCodes[0x96] = &GB::OPCB96;
	CBCodes[0x97] = &GB::OPCB97;
	CBCodes[0x98] = &GB::OPCB98;
	CBCodes[0x99] = &GB::OPCB99;
	CBCodes[0x9A] = &GB::OPCB9A;
	CBCodes[0x9B] = &GB::OPCB9B;
	CBCodes[0x9C] = &GB::OPCB9C;
	CBCodes[0x9D] = &GB::OPCB9D;
	CBCodes[0x9E] = &GB::OPCB9E;
	CBCodes[0x9F] = &GB::OPCB9F;
	CBCodes[0xA0] = &GB::OPCBA0;
	CBCodes[0xA1] = &GB::OPCBA1;
	CBCodes[0xA2] = &GB::OPCBA2;
	CBCodes[0xA3] = &GB::OPCBA3;
	CBCodes[0xA4] = &GB::OPCBA4;
	CBCodes[0xA5] = &GB::OPCBA5;
	CBCodes[0xA6] = &GB::OPCBA6;
	CBCodes[0xA7] = &GB::OPCBA7;
	CBCodes[0xA8] = &GB::OPCBA8;
	CBCodes[0xA9] = &GB::OPCBA9;
	CBCodes[0xAA] = &GB::OPCBAA;
	CBCodes[0xAB] = &GB::OPCBAB;
	CBCodes[0xAC] = &GB::OPCBAC;
	CBCodes[0xAD] = &GB::OPCBAD;
	CBCodes[0xAE] = &GB::OPCBAE;
	CBCodes[0xAF] = &GB::OPCBAF;
	CBCodes[0xB0] = &GB::OPCBB0;
	CBCodes[0xB1] = &GB::OPCBB1;
	CBCodes[0xB2] = &GB::OPCBB2;
	CBCodes[0xB3] = &GB::OPCBB3;
	CBCodes[0xB4] = &GB::OPCBB4;
	CBCodes[0xB5] = &GB::OPCBB5;
	CBCodes[0xB6] = &GB::OPCBB6;
	CBCodes[0xB7] = &GB::OPCBB7;
	CBCodes[0xB8] = &GB::OPCBB8;
	CBCodes[0xB9] = &GB::OPCBB9;
	CBCodes[0xBA] = &GB::OPCBBA;
	CBCodes[0xBB] = &GB::OPCBBB;
	CBCodes[0xBC] = &GB::OPCBBC;
	CBCodes[0xBD] = &GB::OPCBBD;
	CBCodes[0xBE] = &GB::OPCBBE;
	CBCodes[0xBF] = &GB::OPCBBF;
	CBCodes[0xC0] = &GB::OPCBC0;
	CBCodes[0xC1] = &GB::OPCBC1;
	CBCodes[0xC2] = &GB::OPCBC2;
	CBCodes[0xC3] = &GB::OPCBC3;
	CBCodes[0xC4] = &GB::OPCBC4;
	CBCodes[0xC5] = &GB::OPCBC5;
	CBCodes[0xC6] = &GB::OPCBC6;
	CBCodes[0xC7] = &GB::OPCBC7;
	CBCodes[0xC8] = &GB::OPCBC8;
	CBCodes[0xC9] = &GB::OPCBC9;
	CBCodes[0xCA] = &GB::OPCBCA;
	CBCodes[0xCB] = &GB::OPCBCB;
	CBCodes[0xCC] = &GB::OPCBCC;
	CBCodes[0xCD] = &GB::OPCBCD;
	CBCodes[0xCE] = &GB::OPCBCE;
	CBCodes[0xCF] = &GB::OPCBCF;
	CBCodes[0xD0] = &GB::OPCBD0;
	CBCodes[0xD1] = &GB::OPCBD1;
	CBCodes[0xD2] = &GB::OPCBD2;
	CBCodes[0xD3] = &GB::OPCBD3;
	CBCodes[0xD4] = &GB::OPCBD4;
	CBCodes[0xD5] = &GB::OPCBD5;
	CBCodes[0xD6] = &GB::OPCBD6;
	CBCodes[0xD7] = &GB::OPCBD7;
	CBCodes[0xD8] = &GB::OPCBD8;
	CBCodes[0xD9] = &GB::OPCBD9;
	CBCodes[0xDA] = &GB::OPCBDA;
	CBCodes[0xDB] = &GB::OPCBDB;
	CBCodes[0xDC] = &GB::OPCBDC;
	CBCodes[0xDD] = &GB::OPCBDD;
	CBCodes[0xDE] = &GB::OPCBDE;
	CBCodes[0xDF] = &GB::OPCBDF;
	CBCodes[0xE0] = &GB::OPCBE0;
	CBCodes[0xE1] = &GB::OPCBE1;
	CBCodes[0xE2] = &GB::OPCBE2;
	CBCodes[0xE3] = &GB::OPCBE3;
	CBCodes[0xE4] = &GB::OPCBE4;
	CBCodes[0xE5] = &GB::OPCBE5;
	CBCodes[0xE6] = &GB::OPCBE6;
	CBCodes[0xE7] = &GB::OPCBE7;
	CBCodes[0xE8] = &GB::OPCBE8;
	CBCodes[0xE9] = &GB::OPCBE9;
	CBCodes[0xEA] = &GB::OPCBEA;
	CBCodes[0xEB] = &GB::OPCBEB;
	CBCodes[0xEC] = &GB::OPCBEC;
	CBCodes[0xED] = &GB::OPCBED;
	CBCodes[0xEE] = &GB::OPCBEE;
	CBCodes[0xEF] = &GB::OPCBEF;
	CBCodes[0xF0] = &GB::OPCBF0;
	CBCodes[0xF1] = &GB::OPCBF1;
	CBCodes[0xF2] = &GB::OPCBF2;
	CBCodes[0xF3] = &GB::OPCBF3;
	CBCodes[0xF4] = &GB::OPCBF4;
	CBCodes[0xF5] = &GB::OPCBF5;
	CBCodes[0xF6] = &GB::OPCBF6;
	CBCodes[0xF7] = &GB::OPCBF7;
	CBCodes[0xF8] = &GB::OPCBF8;
	CBCodes[0xF9] = &GB::OPCBF9;
	CBCodes[0xFA] = &GB::OPCBFA;
	CBCodes[0xFB] = &GB::OPCBFB;
	CBCodes[0xFC] = &GB::OPCBFC;
	CBCodes[0xFD] = &GB::OPCBFD;
	CBCodes[0xFE] = &GB::OPCBFE;
	CBCodes[0xFF] = &GB::OPCBFF;
}

void PrintHex(int value)
{
	std::cout << std::setfill('0') << std::setw(2) << std::hex << value;
}

void GB::OUTPUTREGISTERS(ui8 op)
{
	std::cout << "*******************************************************" << std::endl;
	std::cout << "Current OPCode is: " << std::hex << OPInstruction[op] << " Instruction: " << "0x" << (int)op << std::endl;

	std::cout << "A Register is ";
	PrintHex(register8bit[A_REGISTER]);
	std::cout << "               F Register is ";
	PrintHex(register8bit[F_REGISTER]);
	std::cout << std::endl;

	std::cout << "B Register is ";
	PrintHex(register8bit[B_REGISTER]);
	std::cout << "               C Register is ";
	PrintHex(register8bit[C_REGISTER]);
	std::cout << std::endl;

	std::cout << "D Register is ";
	PrintHex(register8bit[D_REGISTER]);
	std::cout << "               E Register is ";
	PrintHex(register8bit[E_REGISTER]);
	std::cout << std::endl;

	std::cout << "H Register is "; //AFBC DE HL
	PrintHex(register8bit[H_REGISTER]);
	std::cout << "               L Register is ";
	PrintHex(register8bit[L_REGISTER]);
	std::cout << std::endl;

	std::cout << "AF Register is ";
	PrintHex(register16bit[AF_REGISTER]);
	std::cout << "               BC Register is ";
	PrintHex(register16bit[BC_REGISTER]);
	std::cout << std::endl;

	std::cout << "DE Register is ";
	PrintHex(register16bit[DE_REGISTER]);
	std::cout << "               HL Register is ";
	PrintHex(register16bit[HL_REGISTER]);
	std::cout << std::endl;

	std::cout << "PC Register is ";
	PrintHex(register16bit[PC_REGISTER]);
	std::cout << "               SP Register is ";
	PrintHex(register16bit[SP_REGISTER]);
	std::cout << std::endl << std::endl;

	std::cout << "0xFF11 is ";
	PrintHex(m_bus[0xFF11]);
	std::cout << "                   0xFF12 is ";
	PrintHex(m_bus[0xFF12]);
	std::cout << std::endl;

	std::cout << "0xFF24 is ";
	PrintHex(m_bus[0xFF24]);
	std::cout << "                   0xFF47 is ";
	PrintHex(m_bus[0xFF47]);
	std::cout << std::endl;

	std::cout << "0xFF44 is ";
	PrintHex(m_bus[0xFF44]);
	std::cout << "                   0xFF40 is ";
	PrintHex(m_bus[0xFF40]);
	std::cout << std::endl;

	std::cout << "0xFF42 is ";
	PrintHex(m_bus[0xFF42]);
	std::cout << "                   0xFF85 is ";
	PrintHex(m_bus[0xFF85]);
	std::cout << std::endl;

	std::cout << "0xFF00 is ";
	PrintHex(m_bus[0xFF00]);
	std::cout << std::endl;

	std::cout << std::bitset<8>(register8bit[F_REGISTER]) << "<- Flags" << std::endl;
	std::cout << "ZSHC" << std::endl;

	std::cout << "*******************************************************" << std::endl;
	system("pause");

	std::cout << std::endl;
}

void GB::OUTPUTCBREGISTERS(ui8 op)
{
	std::cout << "*******************************************************" << std::endl;
	std::cout << "Current OPCode is: " << std::hex << CBOPInstruction[op] << " Instruction: " << "0x" << (int)op << std::endl;

	std::cout << "A Register is ";
	PrintHex(register8bit[A_REGISTER]);
	std::cout << "               F Register is ";
	PrintHex(register8bit[F_REGISTER]);
	std::cout << std::endl;

	std::cout << "B Register is ";
	PrintHex(register8bit[B_REGISTER]);
	std::cout << "               C Register is ";
	PrintHex(register8bit[C_REGISTER]);
	std::cout << std::endl;

	std::cout << "D Register is ";
	PrintHex(register8bit[D_REGISTER]);
	std::cout << "               E Register is ";
	PrintHex(register8bit[E_REGISTER]);
	std::cout << std::endl;

	std::cout << "H Register is ";
	PrintHex(register8bit[H_REGISTER]);
	std::cout << "               L Register is ";
	PrintHex(register8bit[L_REGISTER]);
	std::cout << std::endl;

	std::cout << "AF Register is ";
	PrintHex(register16bit[AF_REGISTER]);
	std::cout << "               BC Register is ";
	PrintHex(register16bit[BC_REGISTER]);
	std::cout << std::endl;

	std::cout << "DE Register is ";
	PrintHex(register16bit[DE_REGISTER]);
	std::cout << "               HL Register is ";
	PrintHex(register16bit[HL_REGISTER]);
	std::cout << std::endl;

	std::cout << "PC Register is ";
	PrintHex(register16bit[PC_REGISTER]);
	std::cout << "               SP Register is ";
	PrintHex(register16bit[SP_REGISTER]);
	std::cout << std::endl;

	std::cout << "0xFF11 is ";
	PrintHex(m_bus[0xFF11]);
	std::cout << "                   0xFF12 is ";
	PrintHex(m_bus[0xFF12]);
	std::cout << std::endl;

	std::cout << "0xFF24 is ";
	PrintHex(m_bus[0xFF24]);
	std::cout << "                   0xFF47 is ";
	PrintHex(m_bus[0xFF47]);
	std::cout << std::endl;

	std::cout << "0xFF44 is ";
	PrintHex(m_bus[0xFF44]);
	std::cout << "                   0xFF40 is ";
	PrintHex(m_bus[0xFF40]);
	std::cout << std::endl;

	std::cout << "0xFF42 is ";
	PrintHex(m_bus[0xFF42]);
	std::cout << "                   0xFF85 is ";
	PrintHex(m_bus[0xFF85]);
	std::cout << std::endl;

	std::cout << "0xFF00 is ";
	PrintHex(m_bus[0xFF00]);
	std::cout << std::endl;

	std::cout << std::bitset<8>(register8bit[F_REGISTER]) << std::endl;
	std::cout << "ZSHC" << std::endl;

	std::cout << "*******************************************************" << std::endl;
	system("pause");

	std::cout << std::endl;
}

void GB::JoyPadTick()
{
	m_joypadCycles += cycle;
	if (m_joypadCycles >= joypadCyclesRefresh)
	{
		UpdateJoyPad();
		m_joypadCycles = 0;
	}
}

void GB::UpdateJoyPad()
{
	ui8 current = bus.io.joypad & 0xF0;

	switch (current & 0x30)
	{
	case 0x10:
	{
		ui8 topJoypad = (joypadActual >> 4) & 0x0F;
		current |= topJoypad;
		break;
	}
	case 0x20:
	{
		ui8 bottomJoypad = joypadActual & 0x0F;
		current |= bottomJoypad;
		break;
	}
	case 0x30:
		current |= 0x0F;
		break;
	}

	if ((bus.io.joypad & ~current & 0x0F) != 0)
		RequestInterupt(JOYPAD);

	bus.io.joypad = current;
}

bool GB::createSDLWindow()
{
	//if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	//{
	//	return false;
	//}

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("CrosBOY", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH * windowMultiplier, DISPLAY_HEIGHT * windowMultiplier, SDL_WINDOW_ALLOW_HIGHDPI);

	render = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	screen_texture = SDL_CreateTexture(render, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, DISPLAY_WIDTH, DISPLAY_HEIGHT);

	return true;
}

void GB::DisableLCD()
{
	ui8& line = ReadData(LYRegister);
	//lcdEnabled = false;
	line = 0;
	//currentMode = H_BLANK;

	UpdateLCDStatus();
	ui8& status = ReadData(statusRegister);
	if (HasBit(status, 5))
	{
		RequestInterupt(LCD);
	}
	CompareLYWithLYC();
}

void GB::EnableLCD()
{
	ui8& control = ReadData(lcdcRegister); // Get the LCDC register from the CPU

	if (HasBit(control, 7)) // Is the screen on
	{
		displayEnableDelay = 244;
	}
}

bool GB::updatePixels()
{
	ui8& controlBit = ReadData(lcdcRegister); // Get the LCDC register from the CPU
	ui8& statusControl = ReadData(statusRegister);
	ui8& line = ReadData(LYRegister);

	bool isDisplayEnabled = HasBit(controlBit, 7);

	vBlank = false;

	videoCycles += cycle; //update the cycles (time passed) from the CPU

	//if (lcdEnabled)
	//{
	//	std::cout << "enabled" << std::endl;
	//}
	//else
	//{
	//	std::cout << "disabled" << std::endl;
	//}

	if (isDisplayEnabled && lcdEnabled)
	{

		switch (currentMode)
		{
		case 0:
		{
			if (line == 0x8f)
			{
				//std::cout << "hello" << std::endl;
			}
			handleHBlankMode(line);
		}
		break;
		case 1: 
		{
			handleVBlankMode(line, cycles);
		}
		break;
		case 2:
		{
			handleOAMMode();
		}
		break;
		case 3:
		{
			handleLCDTransferMode();
		}
		break;
		}
	}
	else
	{
		if (displayEnableDelay > 0) //If the display has been enabled and there is still time til it powers on
		{
			displayEnableDelay -= cycle;

			if (displayEnableDelay <= 0)
			{
				lcdEnabled = true;
				displayEnableDelay = 0;
				videoCycles = 0;
				ReadData(LYRegister) = 0;

				currentMode = H_BLANK;
				UpdateLCDStatus();

				if (HasBit(statusControl, 5))
				{
					RequestInterupt(LCD);
				}
				CompareLYWithLYC();
			}
		}
		else if (videoCycles >= 70224) //Fake vBlank
		{
			videoCycles -= 70224;
			vBlank = true;
		}
	}
	return vBlank;
}

void GB::drawScanline()
{
	ui8& line = ReadData(LYRegister);
	ui8& control = ReadData(lcdcRegister); // Get the LCDC register from the CPU

	if (HasBit(control, 7)) // Is the screen on
	{
		if (HasBit(control, 0)) // Is background Enabled
		{
			RenderBackground();

			if (HasBit(control, 5)) // Is the window enabled
			{
				ui8& windowY = ReadData(WINDOW_Y);
				if (line >= windowY)
				{
					RenderWindow(windowY);
				}
			}
		}

		if (HasBit(control, 1)) // Is Sprites Enabled
		{
			RenderSprites();
		}
	}
}

void GB::handleHBlankMode(ui8& line)
{
	if (videoCycles >= MIN_HBLANK_MODE_CYCLES)
	{
		videoCycles -= MIN_HBLANK_MODE_CYCLES;

		currentMode = OAM;

		line++;

		CompareLYWithLYC();

		ui8& lcdStatus = ReadData(lcdcRegister);

		if (line == DISPLAY_HEIGHT)
		{
			currentMode = V_BLANK;

			RequestInterupt(VBLANK);

			//transfer the data to the pixels array?

			//Check bit 4 of the LCDC
			if (HasBit(lcdStatus, 4))
			{
				RequestInterupt(LCD);
			}

			vBlank = true;
		}
		else
		{
			//Check bit 5 of the LCDC
			if (HasBit(lcdStatus, 5))
			{
				RequestInterupt(LCD);
			}
		}

		UpdateLCDStatus();
	}
}

void GB::handleVBlankMode(ui8& line, int cycles)
{
	if (videoCycles >= MAX_VIDEO_CYCLES)
	{
		videoCycles -= MAX_VIDEO_CYCLES;
		line++;
		CompareLYWithLYC();

		if (line > VERTICAL_BLANK_SCAN_LINE_MAX)
		{
			ui8& lcdStatus = ReadData(lcdcRegister);
			line = 0;
			currentMode = OAM;
			UpdateLCDStatus();

			if (HasBit(lcdStatus, 5))
			{
				RequestInterupt(LCD);
			}
		}
	}
}

void GB::handleOAMMode()
{
	if (videoCycles >= MIN_OAM_MODE_CYCLES)
	{
		videoCycles -= MIN_OAM_MODE_CYCLES;

		currentMode = LCD_TRANSFER;

		UpdateLCDStatus();

	}
}

void GB::handleLCDTransferMode()
{
	if (videoCycles >= MIN_LCD_TRANSFER_CYCLES)
	{
		videoCycles -= MIN_LCD_TRANSFER_CYCLES;
		ui8& lcdStatus = ReadData(lcdcRegister);

		drawScanline();

		currentMode = H_BLANK;

		if (HasBit(lcdStatus, 3))
		{
			RequestInterupt(LCD);
		}
	}
}

void GB::RenderGame()
{
	//SDL_UpdateTexture(screen_texture, NULL, frameBuffer, DISPLAY_WIDTH * sizeof(SDL_Colour));
	//SDL_RenderCopy(render, screen_texture, NULL, NULL);
	//SDL_RenderPresent(render);

	void* pixels_ptr;
	int pitch;
	SDL_LockTexture(screen_texture, nullptr, &pixels_ptr, &pitch);

	char* pixels = static_cast<char*>(pixels_ptr);

	memcpy(pixels, frameBuffer, DISPLAY_WIDTH * DISPLAY_HEIGHT * 4);
	SDL_UnlockTexture(screen_texture);
	SDL_RenderCopy(render, screen_texture, nullptr, nullptr);
	SDL_RenderPresent(render);
}

void GB::RenderBackground()
{
	ui8& control = ReadData(lcdcRegister);

	ui8& scrollY = ReadData(SCROLL_Y);
	ui8& scrollX = ReadData(SCROLL_X);

	ui8& line = ReadData(LYRegister);

	bool unsig = HasBit(control, 4); //Which tile set to use 1 or 2 (whether the data is signed or unsigned)

	ui8 palette = ReadData(BACKGROUND_PALLETTE);

	//if (DEBUGGING)
	//{
	//	std::cout << std::dec << (int)(scrollY) << std::endl;
	//}


	ui16 tileData = 0;
	ui16 tileMap = 0;
	//Fetching Data for tile sets
	if (unsig)
	{
		tileData = 0x8000;
	}
	else
	{
		tileData = 0x8800;
	}

	if (HasBit(control, 3)) //Which background data set 
	{
		tileMap = 0x9C00;
	}
	else
	{
		tileMap = 0x9800;
	}

	ui8 xPos = 0;
	ui8 yPos = scrollY + line;

	for (int i = 0; i < DISPLAY_WIDTH; i++)
	{
		xPos = i + scrollX;
		yPos = scrollY + line;
		RenderTile(unsig, tileMap, tileData, xPos, yPos, i, palette);
	}
}

void GB::RenderWindow(ui8 windowY)
{
	ui8& control = ReadData(lcdcRegister);

	ui8& line = ReadData(LYRegister);

	ui8 windowX = ReadData(WINDOW_X);

	if (windowX <= 0x07)
	{
		windowX -= windowX;
	}
	else
	{
		windowX -= 7;
	}

	ui8 palette = ReadData(BACKGROUND_PALLETTE);

	bool unsig = HasBit(control, 4); //Which tile set to use 1 or 2 (whether the data is signed or unsigned)

	ui16 tileData = 0;
	ui16 windowTileMap = 0;
	//Fetching Data for tile sets
	if (unsig)
	{
		tileData = 0x8000;
	}
	else
	{
		tileData = 0x8800;
	}

	if (HasBit(control, 6)) //Which window data set 
	{
		windowTileMap = 0x9C00;
	}
	else
	{
		windowTileMap = 0x9800;
	}

	ui8 xPos = 0;
	ui8 yPos = 0;

	for (int i = 0; i < 160; i++) //For the entire line of pixels
	{
		xPos = i - windowX;
		yPos = line - windowY;
		RenderTile(unsig, windowTileMap, tileData, xPos, yPos, i, palette);
	}
}

void GB::RenderSprites()
{
	ui8& control = ReadData(lcdcRegister);
	ui8& line = ReadData(LYRegister);

	ui8 spriteHeight = 8;

	if (HasBit(control, 2)) //Which window data set 
	{
		spriteHeight = 16;
	}

	for (int sprite = 0; sprite < 40; sprite++)
	{
		//Get the position and Tile index of the sprite
		ui8 index = sprite * 4; //A sprite is 4 bytes ( starting position of the sprite)
		ui8 yPos = ReadData(0xFE00 + index) - 16;
		ui8 xPos = ReadData(0xFE00 + index + 1) - 8;
		ui8 tileIndex = ReadData(0xFE00 + index + 2);

		//Sprite Attributes
		ui8 attributes = ReadData(0xFE00 + index + 3);
		ui16 pallette = HasBit(attributes, 4) ? 0xFF49 : 0xFF48;
		ui8 backgroundPallette = ReadData(0xFF47);
		pixelRGB backgroundZeroColour = currentPallete[getColourFromPallette(backgroundPallette, WHITE)];
		bool xFlip = HasBit(attributes, 5);
		bool yFlip = HasBit(attributes, 6);
		bool spriteOnTop = !HasBit(attributes, 7);

		if ((line >= yPos) && (line < (yPos + spriteHeight))) // TODO: not breaking into this statement to draw the sprites
		{
			int spriteLine = line - yPos;

			if (yFlip) //Change the starting position of the sprite and then reverse it
			{
				spriteLine -= spriteHeight;
				spriteLine *= -1;
			}

			spriteLine *= 2;

			ui16 dataAddress = (0x8000 + (tileIndex * 16)) + spriteLine;
			ui8 upper = ReadData(dataAddress);
			ui8 lower = ReadData(dataAddress + 1);

			//Setting the pixel colours of the sprite tile
			for (int tilePixel = 7; tilePixel >= 0; tilePixel--)
			{
				int xPix = 0 - tilePixel;
				xPix += 7;

				int pixel = xPos + xPix;

				if (pixel < 160 && line < 144)
				{
					int position = tilePixel;
					ui8 colourNum = 0x00;
					if (xFlip)
					{
						position -= 7;
						position *= -1;
					}

					colourNum = (lower >> position) & 1; // Get the set bit
					colourNum <<= 1;
					colourNum |= (upper >> position) & 1;

					if (colourNum != WHITE) // Don't need to draw clear tiles
					{
						int pixelIndex = pixel + (DISPLAY_WIDTH * line);
						//TODO: check if the sprite is on top
						pixelRGB colour = classicPallette[(pallette, colours(colourNum))];
						//Store them in the framebuffer
						frameBuffer[pixelIndex * 4] = colour.blue;
						frameBuffer[pixelIndex * 4 + 1] = colour.green;
						frameBuffer[pixelIndex * 4 + 2] = colour.red;
					}
				}
			}

		}
	}
}

void GB::RenderTile(bool unsig, ui16 tileMap, ui16 tileData, ui8 xPos, ui8 yPos, ui8 pixel, ui8 pallette)
{
	ui8& line = ReadData(LYRegister);
	// which of the 8 vertical pixels of the current tile is the scanline on?
	ui16 tileRow = (((ui8)(yPos / 8)) * 32);
	// Out of the 32 horizontal tiles, what one are we currently on
	ui8 tileColumn = (xPos / 8);
	//Get the address for the tileData
	ui16 tileAddress = tileMap + tileRow + tileColumn;
	ui16 tileLocation = tileData;
	i16 currentTile;

	if (unsig)
	{
		currentTile = (ui8)ReadData(tileAddress);
		tileLocation += (currentTile * 16);
	}
	else
	{
		currentTile = (i8)ReadData(tileAddress);
		tileLocation += ((currentTile + 128) * 16);
	}
	// find which vertical line we're on, each vertical line takes up two bytes of memory
	ui8 vline = (yPos % 8) * 2;
	//Get the upper and lower bytes of tile data (1 bit from each combine to create the colour)
	ui8 upper = ReadData(tileLocation + vline);
	ui8 lower = ReadData(tileLocation + vline + 1);

	int colourBit = xPos % 8;
	colourBit -= 7;
	colourBit = -colourBit;

	//Combining the tile data together to create the value for the pixel colour
	int colourNum = (lower >> colourBit) & 1; // Get the set bit
	colourNum <<= 1;
	colourNum |= (upper >> colourBit) & 1;
	//Visual representation below

	//A            7 6 5 4 3 2 1 0
	//d + ---------------- - +
	//d  0x8000 |  1 0 1 1 0 1 0 1 |
	//r | ---------------- - |
	//e  0x8001 |  0 1 1 0 0 1 0 1 |
	//s + ---------------- - +
	//s            D L W D B W B W
	//"B" for black, "D" for dark-gray, "L" for light-gray and "W" for white.

	int pixelIndex = pixel + (DISPLAY_WIDTH * line);
	//Retrieve the colour values of the pixel from the pallette
	pixelRGB colour = currentPallete[getColourFromPallette(pallette, colours(colourNum))];

	//if (DEBUGGING)
	//{
	//	if (colourNum == BLACK)
	//	{
	//		std::cout << "hello" << std::endl;
	//	}
	//}


	//Store them in the framebuffer
	frameBuffer[pixelIndex * 4] = colour.blue;
	frameBuffer[pixelIndex * 4 + 1] = colour.green;
	frameBuffer[pixelIndex * 4 + 2] = colour.red;
}

colours GB::getColourFromPallette(ui8 pallete, colours originalColour)
{
	ui8 colourNumber = 0;
	switch (originalColour)
	{
	case WHITE:
		if (HasBit(pallete, 1))
		{
			SetBit(colourNumber, 1);
		}
		if (HasBit(pallete, 0))
		{
			SetBit(colourNumber, 0);
		}
		break;
	case LIGHT_GREY:
		if (HasBit(pallete, 3))
		{
			SetBit(colourNumber, 1);
		}
		if (HasBit(pallete, 2))
		{
			SetBit(colourNumber, 0);
		}
		break;
	case DARK_GREY:
		if (HasBit(pallete, 5))
		{
			SetBit(colourNumber, 1);
		}
		if (HasBit(pallete, 4))
		{
			SetBit(colourNumber, 0);
		}
		break;
	case BLACK:
		if (HasBit(pallete, 7))
		{
			SetBit(colourNumber, 1);
		}
		if (HasBit(pallete, 6))
		{
			SetBit(colourNumber, 0);
		}
		break;
	}

	return colours(colourNumber);
}

void GB::DMATransfer(const ui8 data)
{
	// Sprite Data is at data * 100
	ui16 address = data << 8;
	for (int i = 0; i < 0xA0; i++)
	{
		WriteData(0xFE00 + i, ReadData(address + i));
	}
}
