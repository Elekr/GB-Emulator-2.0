#include "GB.h"
#include <assert.h>
#include <iomanip>
#include <bitset>

GB::GB()
{
	InitOPArray();
	InitCBOPArray();

	//Reset function
	SetPC(0x00);
}

bool GB::InitEMU(const char* path)
{
	bool loaded = m_cartridge.Load(path);
	if (!loaded)return false;

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
	// Reset registers
	SetWordRegister(SP_REGISTER, 0x0000);
	SetWordRegister(PC_REGISTER, 0x0000);
	SetWordRegister(AF_REGISTER, 0x0000);
	SetWordRegister(BC_REGISTER, 0x0000);
	SetWordRegister(DE_REGISTER, 0x0000);
	SetWordRegister(HL_REGISTER, 0x0000);


	for (unsigned int i = 0; i < 0xFFFF; ++i)
	{
		m_bus[i] = 0x0;
	}

	// Laod memory bank 0 into memory
	memcpy(m_bus, m_cartridge.GetRawData(), 0x4000);


	// Reset BIOS
	memcpy(m_bus, BIOS, 256);
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
			// To do: Joypad
			break;
		}
		case 0xFF04: // Timer divider
		{
			m_bus[address] = 0x0;
			break;
		}
		case 0xFF07: // Timer Control
		{
			// To do: Timer Control
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
			// To do: DMA transfer
			break;
		}
		case 0xFF50: // Boot rom switch
		{
			// To do, switch from BIOS to cart and back
			m_bus[address] = data;
			break;
		}
		default:
		{
			// Video BG Palette
			// Video Sprite Palette 0
			// Video Sprite Palette 1
			// Video Window Y
			// Video Window X
			// Serial
			// Video LYC
			// Video Status
			// Video Scroll Y
			// Video Scroll X
			// CPU Interrupt Flag
			// Timer
			// Timer Modulo
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

	// Interrupt
	if (address == 0xFFFF) // Low Frequancy
	{
		// To do
		return;
	}





	// Unusable Memory
	/*if (InMemoryRange(0xFEA0, 0xFEFF, address)) // Low Frequancy
	{
		m_bus_memory[address] = data;
		return;
	}*/

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
	IncrementPC();
	//TODO: HALT BUG STUFF
	return result;
}

void GB::IncrementPC()
{
	SetWordRegister(PC_REGISTER, GetWordRegister(PC_REGISTER) + 1); //Increments the PC register by 1
	dynamicPtr++; //Moves the busPtr along one
}

inline void GB::SetPC(const ui16& value)
{
	register16bit[PC_REGISTER] = value;
	dynamicPtr = &m_bus[value];
}

bool GB::HasBit(ui8 data, ui8 bit)
{
	return (data >> bit) & 1;
}

void GB::SetBit(ui8 data, ui8 bit)
{
	data |= 0x1 << bit;
}

void GB::ClearBit(ui8 data, ui8 bit)
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
	SetWordRegister(PC_REGISTER, data);

	dynamicPtr = &m_bus[data];
}

void GB::INCByteRegister(const ui8& reg) //Increments register and sets flags (Z, S, H)
{
	bool hasCarry = CheckFlag(FLAG_CARRY);

	ClearFlags();

	if (hasCarry == true)
	{
		SetFlag(FLAG_CARRY);
	}

	GetByteRegister(reg)++;

	//UnsetFlag(FLAG_SUBTRACT); // Always reset

	if (GetByteRegister(reg) == 0) // Set if 0
	{
		SetFlag(FLAG_ZERO);
	}
	else
	{
		UnsetFlag(FLAG_ZERO);
	}

	if ((GetByteRegister(reg) & 0x0F) == 0x00) // Set if carry from bit 3
	{
		SetFlag(FLAG_HALFCARRY);
	}

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

	if (GetByteRegister(reg) == 0) // Set if 0
	{
		SetFlag(FLAG_ZERO);
	}
	else
	{
		UnsetFlag(FLAG_ZERO);
	}

	SetFlag(FLAG_SUBTRACT);

	if ((GetByteRegister(reg) & 0x0F) == 0x0F) // Set if no borrow from bit 4
	{
		SetFlag(FLAG_HALFCARRY);
	}
	else
	{
		UnsetFlag(FLAG_HALFCARRY);
	}
}

void GB::SetFlag(int flag)
{
	GetByteRegister(F_REGISTER) |= 1 << flag; // Use OR (|) on the flag bit ( 1|0 = 1)
}

void GB::UnsetFlag(int flag)
{
	GetByteRegister(F_REGISTER) &= ~(1 << flag); // Reverse the bits using NOT (~) then use AND (&)
}

bool GB::CheckFlag(int flag)
{
	return(GetByteRegister(F_REGISTER) >> flag) & 1; // Shift the number to the right then AND mask it to 1
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

void GB::ADDHL(const ui16& reg)
{
	int result = GetWordRegister(HL_REGISTER) + reg;

	bool hasZero = CheckFlag(FLAG_ZERO);

	ClearFlags();

	if (!hasZero) // Set if 0
	{
		SetFlag(FLAG_ZERO);
	}
	else
	{
		UnsetFlag(FLAG_ZERO);
	}

	if (result & 0x10000)
	{
		SetFlag(FLAG_CARRY);
	}

	if ((GetWordRegister(HL_REGISTER) ^ reg ^ (result & 0xFFFF)) & 0x1000)
	{
		SetFlag(FLAG_HALFCARRY);
	}

	SetWordRegister(HL_REGISTER, static_cast<ui16>(result));
}

void GB::Bit(const ui8& value, ui8 bit)
{
	ClearFlags();
	//CHECK IF BIT OF THE REGISTER HAS BEEN SET
	if (((value >> bit) & 1) == 0) // If the zero flag has been set
	{
		SetFlag(FLAG_ZERO); // Set in the F register (Dependant)
	}
	else
	{
		UnsetFlag(FLAG_ZERO);
	}

	SetFlag(FLAG_HALFCARRY);
}

void GB::XOR(const ui8& value)
{
	ui8& result = GetByteRegister(A_REGISTER); //XOR the register to the value passed
	result ^= value;

	ClearFlags();

	if (result == 0)
	{
		SetFlag(FLAG_ZERO);
	}
	else
	{
		UnsetFlag(FLAG_ZERO);
	}
}

void GB::LDI(const ui16 address, const ui8& reg)
{
	WriteData(address, GetByteRegister(reg));
	GetWordRegister(HL_REGISTER)++;
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
		SetFlag(FLAG_CARRY);
	}

	reg <<= 1;
	reg |= carry;

	if (!A) // If RL, N variant
	{
		if (reg == 0) // Set if 0
		{
			SetFlag(FLAG_ZERO);
		}
		else
		{
			UnsetFlag(FLAG_ZERO);
		}
	}
}

void GB::RLC(ui8& reg, bool A)
{
	//Similar to the RL function but sets the 7th bit to the carry

	ClearFlags();

	if ((reg & 0x80) != 0) // if it isn't 0 must be 1 (so carry)
	{
		SetFlag(FLAG_CARRY);
		reg <<= 1;
		reg |= 0x1;
	}
	else
	{
		reg <<= 1;
	}

	if (!A)
	{
		if (GetByteRegister(reg) == 0) // Set if 0
		{
			SetFlag(FLAG_ZERO);
		}
		else
		{
			UnsetFlag(FLAG_ZERO);
		}
	}
}

void GB::RR(ui8& reg, bool A)
{
	ui8 carry = CheckFlag(FLAG_CARRY) ? 0x80 : 0x00;

	ClearFlags();
	if ((reg & 0x01) != 0)
	{
		SetFlag(FLAG_CARRY);
	}

	reg >>= 1;
	reg |= carry;

	if (!A)
	{
		if (GetByteRegister(reg) == 0) // Set if 0
		{
			SetFlag(FLAG_ZERO);
		}
		else
		{
			UnsetFlag(FLAG_ZERO);
		}
	}
}

void GB::RRC(ui8& reg, bool A)
{
	ClearFlags();
	if ((reg & 0x01) != 0)
	{
		SetFlag(FLAG_CARRY);
		reg >>= 1;
		reg |= 0x80;
	}
	else
	{
		reg >>= 1;
	}
}

void GB::CP(const ui8& value)
{
	ui8& reg = GetByteRegister(A_REGISTER); //store the old A value (A Remains unchanged)

	SetFlag(FLAG_SUBTRACT);

	if (reg == value)
	{
		SetFlag(FLAG_ZERO);
	}
	else
	{
		UnsetFlag(FLAG_ZERO);
	}

	if (((reg - value) & 15) > (reg & 15))
	{
		SetFlag(FLAG_HALFCARRY);
	}
	else
	{
		UnsetFlag(FLAG_HALFCARRY);
	}

	if (reg < value)
	{
		SetFlag(FLAG_CARRY);
	}
	else
	{
		UnsetFlag(FLAG_CARRY);
	}
}

void GB::SUB(const ui8& value)
{
	int current_register = GetByteRegister(A_REGISTER);
	int result = current_register - value;
	int carrybits = current_register ^ value ^ result;

	SetByteRegister(A_REGISTER, static_cast<ui8>(result));

	ClearFlags();

	SetFlag(FLAG_SUBTRACT);

	if (static_cast<ui8>(result) == 0)
	{
		SetFlag(FLAG_ZERO);
	}
	else
	{
		UnsetFlag(FLAG_ZERO);
	}

	if ((carrybits & 0x100) != 0)
	{
		SetFlag(FLAG_CARRY);
	}
	else
	{
		UnsetFlag(FLAG_CARRY);
	}

	if ((carrybits & 0x10) != 0)
	{
		SetFlag(FLAG_HALFCARRY);
	}
	else
	{
		UnsetFlag(FLAG_HALFCARRY);
	}
}

int GB::TickCPU()
{
	bool vSync = false;

	while (!vSync)
	{
		OPCode = ReadNextCode();

		cycle = (normalCycles[OPCode] * 4);
		cycles += cycle;

		if (OPCode == 0xCB) // Extra Codes
		{
			OPCode = ReadNextCode();

			cycle = (CBCycles[OPCode] * 4);
			cycles += cycle;

			(this->*CBCodes[OPCode])();
			if (GetWordRegister(PC_REGISTER) >= 0x6C && GetWordRegister(PC_REGISTER) < 0x95)
			{
				OUTPUTCBREGISTERS(OPCode);
			}
		}
		else
		{
			(this->*BASECodes[OPCode])();
			if (GetWordRegister(PC_REGISTER) >= 0x6C && GetWordRegister(PC_REGISTER) < 0x95)
			{
				OUTPUTREGISTERS(OPCode);
			}
		}
		vSync = updatePixels();
	}
	return 0;
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

void GB::RequestInterupt(CPUInterupt interupt)
{
	ReadData(0xFF0F) |= 1 << (ui8)interupt;
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

	if (interuptsToProcess > 0)
	{
		if (interruptsEnabled)
		{
			// Loop through for all possible interrupts 
			for (int i = 0; i < 5; i++)
			{
				if (HasBit(interuptsToProcess, i))
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
					}
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
	ui8& controlBit = ReadData(lcdcRegister); // Get the LCDC register from the CPU
	ui8& line = ReadData(LYRegister);

	bool isDisplayEnabled = HasBit(lcdcRegister, 7);

	if (isDisplayEnabled)
	{
		ui8& status = ReadData(statusRegister);
		ui8& LYC = ReadData(lycRegister);

		if (LYC == line)
		{
			SetBit(statusRegister, 2);
			if (HasBit(status, 6))
			{
				//LCD Interrupt
			}
		}
		else
		{
			ClearBit(statusRegister, 2);
		}
	}
}

//OP CODES
void GB::OP00() {}; // NOP
void GB::OP01() { SetWordRegister(BC_REGISTER, ReadWord()); }; // LD BC, nn 
void GB::OP02() { WriteData(GetWordRegister(BC_REGISTER), GetByteRegister(A_REGISTER)); }; // LD (BC), A
void GB::OP03() { GetWordRegister(BC_REGISTER)++; }; // INC BC
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

	WriteData(pc, low);
	WriteData(pc + 1, high);
}; // LD (nn) SP
void GB::OP09() { ADDHL(GetWordRegister(BC_REGISTER)); }; // ADD HL, BC
void GB::OP0A() { SetByteRegister(A_REGISTER, ReadData(GetWordRegister(BC_REGISTER))); }; // LD A, (BC)
void GB::OP0B() { GetWordRegister(BC_REGISTER)--; }; // DEC BC
void GB::OP0C() { INCByteRegister(C_REGISTER); }; // INC C
void GB::OP0D() { DECByteRegister(C_REGISTER); }; // DEC C
void GB::OP0E() { SetByteRegister(C_REGISTER, ReadByte()); }; // LD C, ui8
void GB::OP0F() { RRC(GetByteRegister(A_REGISTER), true); }; // RRCA
void GB::OP10() { IncrementPC(); }; // STOP (does nothing unless gbc
void GB::OP11() { SetWordRegister(DE_REGISTER, ReadWord()); }; // LD DE, nn
void GB::OP12() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(A_REGISTER)); }; // LD (DE), A
void GB::OP13() { GetWordRegister(DE_REGISTER)++; }; // INC DE
void GB::OP14() { INCByteRegister(D_REGISTER); }; // INC D
void GB::OP15() { DECByteRegister(D_REGISTER); }; // DEC D
void GB::OP16() { SetByteRegister(D_REGISTER, ReadByte()); }; // LD D, ui8
void GB::OP17() { RL(GetByteRegister(A_REGISTER), true); }; // RLA
void GB::OP18() { Jr(); }; // Jr i8
void GB::OP19() { ADDHL(GetWordRegister(DE_REGISTER)); }; // ADD HL, DE
void GB::OP1A() { SetByteRegister(A_REGISTER, ReadData(GetWordRegister(DE_REGISTER))); }; // LD A, (DE)
void GB::OP1B() { GetWordRegister(DE_REGISTER)++; }; // DEC DE
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
void GB::OP23() { GetWordRegister(HL_REGISTER)++; }; // INC HL
void GB::OP24() { INCByteRegister(H_REGISTER); }; // INC H
void GB::OP25() { DECByteRegister(H_REGISTER); }; // DEC H
void GB::OP26() { SetByteRegister(H_REGISTER, ReadByte()); }; // LD H, ui8
void GB::OP27() { assert("Missing" && 0); }; //
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
void GB::OP2A() { LDI(A_REGISTER, GetWordRegister(HL_REGISTER)); }; // LD A, (HL+)
void GB::OP2B() { GetWordRegister(HL_REGISTER)--; }; // DEC HL
void GB::OP2C() { INCByteRegister(L_REGISTER); }; // INC L
void GB::OP2D() { DECByteRegister(L_REGISTER); }; // DEC L
void GB::OP2E() { SetByteRegister(L_REGISTER, ReadByte()); }; // LD L, ui8
void GB::OP2F() 
{
	SetByteRegister(A_REGISTER, ~GetByteRegister(A_REGISTER)); // Flips the bits of the register

	SetFlag(FLAG_HALFCARRY);
	SetFlag(FLAG_SUBTRACT);
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
}; // LD (HL--), A https://blog.tigris.fr/2019/07/28/writing-an-emulator-memory-management/
void GB::OP33() { GetWordRegister(SP_REGISTER)--; }; // INC SP
void GB::OP34() 
{
	ui8 byte = ReadData(HL_REGISTER); // store the data
	byte++; // inc
	WriteData(GetWordRegister(HL_REGISTER), byte);
}; // INC (HL)
void GB::OP35()
{
	ui8 byte = ReadData(HL_REGISTER); // store the data
	byte++; // inc
	WriteData(GetWordRegister(HL_REGISTER), byte);
} // return data back}; // DEC (HL)
void GB::OP36() { WriteData(GetWordRegister(HL_REGISTER), ReadByte()); }; // LD (HL) ui8
void GB::OP37() 
{
	SetFlag(FLAG_CARRY);
	UnsetFlag(FLAG_HALFCARRY);
	UnsetFlag(FLAG_SUBTRACT);
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
}; // LD A, (HL-)
void GB::OP3B() { GetWordRegister(SP_REGISTER)--; }; // DEC SP
void GB::OP3C() { INCByteRegister(A_REGISTER); }; // INC A
void GB::OP3D() { DECByteRegister(A_REGISTER); }; // DEC A
void GB::OP3E() { SetByteRegister(A_REGISTER, ReadByte()); }; // LD A, ui8
void GB::OP3F() 
{
	bool flag = CheckFlag(FLAG_CARRY);

	if (flag)
	{
		UnsetFlag(FLAG_CARRY);
	}
	else
	{
		SetFlag(FLAG_CARRY);
	}

	UnsetFlag(FLAG_HALFCARRY);
	UnsetFlag(FLAG_SUBTRACT);
}; // CCF http://z80-heaven.wikidot.com/instructions-set:ccf
void GB::OP40() { SetByteRegister(B_REGISTER, GetByteRegister(B_REGISTER)); }; // LD B, B
void GB::OP41() { SetByteRegister(B_REGISTER, GetByteRegister(C_REGISTER)); }; // LD B, C
void GB::OP42() { SetByteRegister(B_REGISTER, GetByteRegister(D_REGISTER)); }; // LD B, D
void GB::OP43() { SetByteRegister(B_REGISTER, GetByteRegister(E_REGISTER)); }; // LD B, E
void GB::OP44() { SetByteRegister(B_REGISTER, GetByteRegister(H_REGISTER)); }; // LD B, H
void GB::OP45() { SetByteRegister(B_REGISTER, GetByteRegister(L_REGISTER)); }; // LD B, L
void GB::OP46() { SetByteRegister(B_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); }; // LD B, HL
void GB::OP47() { SetByteRegister(B_REGISTER, GetByteRegister(A_REGISTER)); }; // LD B, A
void GB::OP48() { SetByteRegister(C_REGISTER, GetByteRegister(B_REGISTER)); }; // LD C, B
void GB::OP49() { SetByteRegister(C_REGISTER, GetByteRegister(C_REGISTER)); }; // LD C, C
void GB::OP4A() { SetByteRegister(C_REGISTER, GetByteRegister(D_REGISTER)); }; // LD C, D
void GB::OP4B() { SetByteRegister(C_REGISTER, GetByteRegister(E_REGISTER)); }; // LD C, E
void GB::OP4C() { SetByteRegister(C_REGISTER, GetByteRegister(H_REGISTER)); }; // LD C, H
void GB::OP4D() { SetByteRegister(C_REGISTER, GetByteRegister(L_REGISTER)); }; // LD C, L
void GB::OP4E() { SetByteRegister(C_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); }; // LD C, HL
void GB::OP4F() { SetByteRegister(C_REGISTER, GetByteRegister(A_REGISTER)); }; // LD C, A
void GB::OP50() {assert("Missing" && 0);};
void GB::OP51() {assert("Missing" && 0);};
void GB::OP52() {assert("Missing" && 0);};
void GB::OP53() {assert("Missing" && 0);};
void GB::OP54() {assert("Missing" && 0);};
void GB::OP55() {assert("Missing" && 0);};
void GB::OP56() {assert("Missing" && 0);};
void GB::OP57() { SetByteRegister(D_REGISTER, GetByteRegister(A_REGISTER)); }; //LD D, A
void GB::OP58() {assert("Missing" && 0);};
void GB::OP59() {assert("Missing" && 0);};
void GB::OP5A() {assert("Missing" && 0);};
void GB::OP5B() {assert("Missing" && 0);};
void GB::OP5C() {assert("Missing" && 0);};
void GB::OP5D() {assert("Missing" && 0);};
void GB::OP5E() {assert("Missing" && 0);};
void GB::OP5F() { SetByteRegister(E_REGISTER, GetByteRegister(A_REGISTER)); }; // LD E, A
void GB::OP60() {assert("Missing" && 0);};
void GB::OP61() {assert("Missing" && 0);};
void GB::OP62() {assert("Missing" && 0);};
void GB::OP63() {assert("Missing" && 0);};
void GB::OP64() {assert("Missing" && 0);};
void GB::OP65() {assert("Missing" && 0);};
void GB::OP66() {assert("Missing" && 0);};
void GB::OP67() { SetByteRegister(H_REGISTER, GetByteRegister(A_REGISTER)); }; // LD H, A
void GB::OP68() {assert("Missing" && 0);};
void GB::OP69() {assert("Missing" && 0);};
void GB::OP6A() {assert("Missing" && 0);};
void GB::OP6B() {assert("Missing" && 0);};
void GB::OP6C() {assert("Missing" && 0);};
void GB::OP6D() {assert("Missing" && 0);};
void GB::OP6E() {assert("Missing" && 0);};
void GB::OP6F() { SetByteRegister(L_REGISTER, GetByteRegister(A_REGISTER)); }; // LD L, A
void GB::OP70() {assert("Missing" && 0);};
void GB::OP71() {assert("Missing" && 0);};
void GB::OP72() {assert("Missing" && 0);};
void GB::OP73() {assert("Missing" && 0);};
void GB::OP74() {assert("Missing" && 0);};
void GB::OP75() {assert("Missing" && 0);};
void GB::OP76() {assert("Missing" && 0);};
void GB::OP77() { WriteData(GetWordRegister(HL_REGISTER), GetByteRegister(A_REGISTER)); }; // LD (HL), A
void GB::OP78() {assert("Missing" && 0);};
void GB::OP79() {assert("Missing" && 0);};
void GB::OP7A() {assert("Missing" && 0);};
void GB::OP7B() { SetByteRegister(A_REGISTER, GetByteRegister(E_REGISTER)); };
void GB::OP7C() { SetByteRegister(A_REGISTER, GetByteRegister(H_REGISTER)); }; // LD A, H
void GB::OP7D() { SetByteRegister(A_REGISTER, GetByteRegister(L_REGISTER)); }; // A, L
void GB::OP7E() { SetByteRegister(A_REGISTER, ReadData(GetWordRegister(HL_REGISTER))); }; // LD A, (HL)
void GB::OP7F() { SetByteRegister(A_REGISTER, GetByteRegister(A_REGISTER)); }; // LD A, A
void GB::OP80() {assert("Missing" && 0);};
void GB::OP81() {assert("Missing" && 0);};
void GB::OP82() {assert("Missing" && 0);};
void GB::OP83() {assert("Missing" && 0);};
void GB::OP84() {assert("Missing" && 0);};
void GB::OP85() {assert("Missing" && 0);};
void GB::OP86() {assert("Missing" && 0);};
void GB::OP87() {assert("Missing" && 0);};
void GB::OP88() {assert("Missing" && 0);};
void GB::OP89() {assert("Missing" && 0);};
void GB::OP8A() {assert("Missing" && 0);};
void GB::OP8B() {assert("Missing" && 0);};
void GB::OP8C() {assert("Missing" && 0);};
void GB::OP8D() {assert("Missing" && 0);};
void GB::OP8E() {assert("Missing" && 0);};
void GB::OP8F() {assert("Missing" && 0);};
void GB::OP90() { SUB(GetByteRegister(B_REGISTER)); }; // SUB B
void GB::OP91() {assert("Missing" && 0);};
void GB::OP92() {assert("Missing" && 0);};
void GB::OP93() {assert("Missing" && 0);};
void GB::OP94() {assert("Missing" && 0);};
void GB::OP95() {assert("Missing" && 0);};
void GB::OP96() {assert("Missing" && 0);};
void GB::OP97() {assert("Missing" && 0);};
void GB::OP98() {assert("Missing" && 0);};
void GB::OP99() {assert("Missing" && 0);};
void GB::OP9A() {assert("Missing" && 0);};
void GB::OP9B() {assert("Missing" && 0);};
void GB::OP9C() {assert("Missing" && 0);};
void GB::OP9D() {assert("Missing" && 0);};
void GB::OP9E() {assert("Missing" && 0);};
void GB::OP9F() {assert("Missing" && 0);};
void GB::OPA0() {assert("Missing" && 0);};
void GB::OPA1() {assert("Missing" && 0);};
void GB::OPA2() {assert("Missing" && 0);};
void GB::OPA3() {assert("Missing" && 0);};
void GB::OPA4() {assert("Missing" && 0);};
void GB::OPA5() {assert("Missing" && 0);};
void GB::OPA6() {assert("Missing" && 0);};
void GB::OPA7() {assert("Missing" && 0);};
void GB::OPA8() { XOR(GetByteRegister(B_REGISTER)); }; // XOR A, B
void GB::OPA9() { XOR(GetByteRegister(C_REGISTER)); }; // XOR A, C
void GB::OPAA() { XOR(GetByteRegister(D_REGISTER)); }; // XOR A, D
void GB::OPAB() { XOR(GetByteRegister(E_REGISTER)); }; // XOR A, E
void GB::OPAC() { XOR(GetByteRegister(H_REGISTER)); }; // XOR A, H
void GB::OPAD() { XOR(GetByteRegister(L_REGISTER)); }; // XOR A, L
void GB::OPAE() { assert("Missing" && 0); };
void GB::OPAF() { XOR(GetByteRegister(A_REGISTER)); }; // XOR A, A
void GB::OPB0() {assert("Missing" && 0);};
void GB::OPB1() {assert("Missing" && 0);};
void GB::OPB2() {assert("Missing" && 0);};
void GB::OPB3() {assert("Missing" && 0);};
void GB::OPB4() {assert("Missing" && 0);};
void GB::OPB5() {assert("Missing" && 0);};
void GB::OPB6() {assert("Missing" && 0);};
void GB::OPB7() {assert("Missing" && 0);};
void GB::OPB8() {assert("Missing" && 0);};
void GB::OPB9() {assert("Missing" && 0);};
void GB::OPBA() {assert("Missing" && 0);};
void GB::OPBB() {assert("Missing" && 0);};
void GB::OPBC() {assert("Missing" && 0);};
void GB::OPBD() {assert("Missing" && 0);};
void GB::OPBE() { CP(ReadData(GetWordRegister(HL_REGISTER))); }; // CP A, (HL)
void GB::OPBF() {assert("Missing" && 0);};
void GB::OPC0() {assert("Missing" && 0);};
void GB::OPC1() { PopStack(BC_REGISTER); }; // POP BC
void GB::OPC2() {assert("Missing" && 0);};
void GB::OPC3() {assert("Missing" && 0);};
void GB::OPC4() {assert("Missing" && 0);};
void GB::OPC5() { PushStack(BC_REGISTER); }; // PUSH BC
void GB::OPC6() {assert("Missing" && 0);};
void GB::OPC7() {assert("Missing" && 0);};
void GB::OPC8() {assert("Missing" && 0);};
void GB::OPC9() { PopStackPC(); }; // RET
void GB::OPCA() {assert("Missing" && 0);};
void GB::OPCB() {assert("Missing" && 0);};
void GB::OPCC() {assert("Missing" && 0);};
void GB::OPCD()
{
	ui16 word = ReadWord(); //Get the address 
	PushStack(PC_REGISTER); // Stores PC onto SP
	SetPC(word); // Move to the address};
} // CALL u16 (Push address of next instruction onto stack and then jump to address)
void GB::OPCE() {assert("Missing" && 0);};
void GB::OPCF() {assert("Missing" && 0);};
void GB::OPD0() {assert("Missing" && 0);};
void GB::OPD1() { PopStack(DE_REGISTER); }; // POP DE
void GB::OPD2() {assert("Missing" && 0);};
void GB::OPD3() {assert("Missing" && 0);};
void GB::OPD4() {assert("Missing" && 0);};
void GB::OPD5() { PushStack(DE_REGISTER); }; // PUSH DE
void GB::OPD6() {assert("Missing" && 0);};
void GB::OPD7() {assert("Missing" && 0);};
void GB::OPD8() {assert("Missing" && 0);};
void GB::OPD9() {assert("Missing" && 0);};
void GB::OPDA() {assert("Missing" && 0);};
void GB::OPDB() {assert("Missing" && 0);};
void GB::OPDC() {assert("Missing" && 0);};
void GB::OPDD() {assert("Missing" && 0);};
void GB::OPDE() {assert("Missing" && 0);};
void GB::OPDF() {assert("Missing" && 0);};
void GB::OPE0() { WriteData((0xFF00 + ReadByte()), GetByteRegister(A_REGISTER)); }; // LD (FF00+UI8), A
void GB::OPE1() { PopStack(HL_REGISTER); }; // POP HL
void GB::OPE2() { WriteData(0xff00 + GetByteRegister(C_REGISTER), GetByteRegister(A_REGISTER)); }; // LD (FF00 + C), A
void GB::OPE3() {assert("Missing" && 0);};
void GB::OPE4() {assert("Missing" && 0);};
void GB::OPE5() { PushStack(HL_REGISTER); }; // PUSH HL
void GB::OPE6() {assert("Missing" && 0);};
void GB::OPE7() {assert("Missing" && 0);};
void GB::OPE8() {assert("Missing" && 0);};
void GB::OPE9() {assert("Missing" && 0);};
void GB::OPEA() { WriteData(ReadWord(), GetByteRegister(A_REGISTER)); };
void GB::OPEB() {assert("Missing" && 0);};
void GB::OPEC() {assert("Missing" && 0);};
void GB::OPED() {assert("Missing" && 0);};
void GB::OPEE() {assert("Missing" && 0);};
void GB::OPEF() {assert("Missing" && 0);};
void GB::OPF0() { SetByteRegister(A_REGISTER, ReadData(static_cast<ui16> (0xFF00 + ReadByte()))); };
void GB::OPF1() {assert("Missing" && 0);};
void GB::OPF2() {assert("Missing" && 0);};
void GB::OPF3() {assert("Missing" && 0);};
void GB::OPF4() {assert("Missing" && 0);};
void GB::OPF5() { PushStack(AF_REGISTER); }; // PUSH AF
void GB::OPF6() {assert("Missing" && 0);};
void GB::OPF7() {assert("Missing" && 0);};
void GB::OPF8() {assert("Missing" && 0);};
void GB::OPF9() {assert("Missing" && 0);};
void GB::OPFA() {assert("Missing" && 0);};
void GB::OPFB()
{
	interruptsEnabled = true; //Enables interrupts (presumably mode 1 on the z80?) http://jgmalcolm.com/z80/advanced/im1i
} // EI
void GB::OPFC() {};
void GB::OPFD() {};
void GB::OPFE() { CP(ReadByte()); }; // CP, n
void GB::OPFF() {};

//CB OP CODES
void GB::OPCB00() {assert("Missing" && 0);};
void GB::OPCB01() {assert("Missing" && 0);};
void GB::OPCB02() {assert("Missing" && 0);};
void GB::OPCB03() {assert("Missing" && 0);};
void GB::OPCB04() {assert("Missing" && 0);};
void GB::OPCB05() {assert("Missing" && 0);};
void GB::OPCB06() {assert("Missing" && 0);};
void GB::OPCB07() {assert("Missing" && 0);};
void GB::OPCB08() {assert("Missing" && 0);};
void GB::OPCB09() {assert("Missing" && 0);};
void GB::OPCB0A() {assert("Missing" && 0);};
void GB::OPCB0B() {assert("Missing" && 0);};
void GB::OPCB0C() {assert("Missing" && 0);};
void GB::OPCB0D() {assert("Missing" && 0);};
void GB::OPCB0E() {assert("Missing" && 0);};
void GB::OPCB0F() {assert("Missing" && 0);};
void GB::OPCB10() {assert("Missing" && 0);};
void GB::OPCB11() { RL(GetByteRegister(C_REGISTER), false); };
void GB::OPCB12() {assert("Missing" && 0);};
void GB::OPCB13() {assert("Missing" && 0);};
void GB::OPCB14() {assert("Missing" && 0);};
void GB::OPCB15() {assert("Missing" && 0);};
void GB::OPCB16() {assert("Missing" && 0);};
void GB::OPCB17() {assert("Missing" && 0);};
void GB::OPCB18() {assert("Missing" && 0);};
void GB::OPCB19() {assert("Missing" && 0);};
void GB::OPCB1A() {assert("Missing" && 0);};
void GB::OPCB1B() {assert("Missing" && 0);};
void GB::OPCB1C() {assert("Missing" && 0);};
void GB::OPCB1D() {assert("Missing" && 0);};
void GB::OPCB1E() {assert("Missing" && 0);};
void GB::OPCB1F() {assert("Missing" && 0);};
void GB::OPCB20() {assert("Missing" && 0);};
void GB::OPCB21() {assert("Missing" && 0);};
void GB::OPCB22() {assert("Missing" && 0);};
void GB::OPCB23() {assert("Missing" && 0);};
void GB::OPCB24() {assert("Missing" && 0);};
void GB::OPCB25() {assert("Missing" && 0);};
void GB::OPCB26() {assert("Missing" && 0);};
void GB::OPCB27() {assert("Missing" && 0);};
void GB::OPCB28() {assert("Missing" && 0);};
void GB::OPCB29() {assert("Missing" && 0);};
void GB::OPCB2A() {assert("Missing" && 0);};
void GB::OPCB2B() {assert("Missing" && 0);};
void GB::OPCB2C() {assert("Missing" && 0);};
void GB::OPCB2D() {assert("Missing" && 0);};
void GB::OPCB2E() {assert("Missing" && 0);};
void GB::OPCB2F() {assert("Missing" && 0);};
void GB::OPCB30() {assert("Missing" && 0);};
void GB::OPCB31() {assert("Missing" && 0);};
void GB::OPCB32() {assert("Missing" && 0);};
void GB::OPCB33() {assert("Missing" && 0);};
void GB::OPCB34() {assert("Missing" && 0);};
void GB::OPCB35() {assert("Missing" && 0);};
void GB::OPCB36() {assert("Missing" && 0);};
void GB::OPCB37() {assert("Missing" && 0);};
void GB::OPCB38() {assert("Missing" && 0);};
void GB::OPCB39() {assert("Missing" && 0);};
void GB::OPCB3A() {assert("Missing" && 0);};
void GB::OPCB3B() {assert("Missing" && 0);};
void GB::OPCB3C() {assert("Missing" && 0);};
void GB::OPCB3D() {assert("Missing" && 0);};
void GB::OPCB3E() {assert("Missing" && 0);};
void GB::OPCB3F() {assert("Missing" && 0);};
void GB::OPCB40() {assert("Missing" && 0);};
void GB::OPCB41() {assert("Missing" && 0);};
void GB::OPCB42() {assert("Missing" && 0);};
void GB::OPCB43() {assert("Missing" && 0);};
void GB::OPCB44() {assert("Missing" && 0);};
void GB::OPCB45() {assert("Missing" && 0);};
void GB::OPCB46() {assert("Missing" && 0);};
void GB::OPCB47() {assert("Missing" && 0);};
void GB::OPCB48() {assert("Missing" && 0);};
void GB::OPCB49() {assert("Missing" && 0);};
void GB::OPCB4A() {assert("Missing" && 0);};
void GB::OPCB4B() {assert("Missing" && 0);};
void GB::OPCB4C() {assert("Missing" && 0);};
void GB::OPCB4D() {assert("Missing" && 0);};
void GB::OPCB4E() {assert("Missing" && 0);};
void GB::OPCB4F() {assert("Missing" && 0);};
void GB::OPCB50() {assert("Missing" && 0);};
void GB::OPCB51() {assert("Missing" && 0);};
void GB::OPCB52() {assert("Missing" && 0);};
void GB::OPCB53() {assert("Missing" && 0);};
void GB::OPCB54() {assert("Missing" && 0);};
void GB::OPCB55() {assert("Missing" && 0);};
void GB::OPCB56() {assert("Missing" && 0);};
void GB::OPCB57() {assert("Missing" && 0);};
void GB::OPCB58() {assert("Missing" && 0);};
void GB::OPCB59() {assert("Missing" && 0);};
void GB::OPCB5A() {assert("Missing" && 0);};
void GB::OPCB5B() {assert("Missing" && 0);};
void GB::OPCB5C() {assert("Missing" && 0);};
void GB::OPCB5D() {assert("Missing" && 0);};
void GB::OPCB5E() {assert("Missing" && 0);};
void GB::OPCB5F() {assert("Missing" && 0);};
void GB::OPCB60() {assert("Missing" && 0);};
void GB::OPCB61() {assert("Missing" && 0);};
void GB::OPCB62() {assert("Missing" && 0);};
void GB::OPCB63() {assert("Missing" && 0);};
void GB::OPCB64() {assert("Missing" && 0);};
void GB::OPCB65() {assert("Missing" && 0);};
void GB::OPCB66() {assert("Missing" && 0);};
void GB::OPCB67() {assert("Missing" && 0);};
void GB::OPCB68() {assert("Missing" && 0);};
void GB::OPCB69() {assert("Missing" && 0);};
void GB::OPCB6A() {assert("Missing" && 0);};
void GB::OPCB6B() {assert("Missing" && 0);};
void GB::OPCB6C() {assert("Missing" && 0);};
void GB::OPCB6D() {assert("Missing" && 0);};
void GB::OPCB6E() {assert("Missing" && 0);};
void GB::OPCB6F() {assert("Missing" && 0);};
void GB::OPCB70() {assert("Missing" && 0);};
void GB::OPCB71() {assert("Missing" && 0);};
void GB::OPCB72() {assert("Missing" && 0);};
void GB::OPCB73() {assert("Missing" && 0);};
void GB::OPCB74() {assert("Missing" && 0);};
void GB::OPCB75() {assert("Missing" && 0);};
void GB::OPCB76() {assert("Missing" && 0);};
void GB::OPCB77() {assert("Missing" && 0);};
void GB::OPCB78() {assert("Missing" && 0);};
void GB::OPCB79() {assert("Missing" && 0);};
void GB::OPCB7A() {assert("Missing" && 0);};
void GB::OPCB7B() {assert("Missing" && 0);};
void GB::OPCB7C() { Bit(GetByteRegister(H_REGISTER), 7); };
void GB::OPCB7D() {assert("Missing" && 0);};
void GB::OPCB7E() {assert("Missing" && 0);};
void GB::OPCB7F() {assert("Missing" && 0);};
void GB::OPCB80() {assert("Missing" && 0);};
void GB::OPCB81() {assert("Missing" && 0);};
void GB::OPCB82() {assert("Missing" && 0);};
void GB::OPCB83() {assert("Missing" && 0);};
void GB::OPCB84() {assert("Missing" && 0);};
void GB::OPCB85() {assert("Missing" && 0);};
void GB::OPCB86() {assert("Missing" && 0);};
void GB::OPCB87() {assert("Missing" && 0);};
void GB::OPCB88() {assert("Missing" && 0);};
void GB::OPCB89() {assert("Missing" && 0);};
void GB::OPCB8A() {assert("Missing" && 0);};
void GB::OPCB8B() {assert("Missing" && 0);};
void GB::OPCB8C() {assert("Missing" && 0);};
void GB::OPCB8D() {assert("Missing" && 0);};
void GB::OPCB8E() {assert("Missing" && 0);};
void GB::OPCB8F() {assert("Missing" && 0);};
void GB::OPCB90() {assert("Missing" && 0);};
void GB::OPCB91() {assert("Missing" && 0);};
void GB::OPCB92() {assert("Missing" && 0);};
void GB::OPCB93() {assert("Missing" && 0);};
void GB::OPCB94() {assert("Missing" && 0);};
void GB::OPCB95() {assert("Missing" && 0);};
void GB::OPCB96() {assert("Missing" && 0);};
void GB::OPCB97() {assert("Missing" && 0);};
void GB::OPCB98() {assert("Missing" && 0);};
void GB::OPCB99() {assert("Missing" && 0);};
void GB::OPCB9A() {assert("Missing" && 0);};
void GB::OPCB9B() {assert("Missing" && 0);};
void GB::OPCB9C() {assert("Missing" && 0);};
void GB::OPCB9D() {assert("Missing" && 0);};
void GB::OPCB9E() {assert("Missing" && 0);};
void GB::OPCB9F() {assert("Missing" && 0);};
void GB::OPCBA0() {assert("Missing" && 0);};
void GB::OPCBA1() {assert("Missing" && 0);};
void GB::OPCBA2() {assert("Missing" && 0);};
void GB::OPCBA3() {assert("Missing" && 0);};
void GB::OPCBA4() {assert("Missing" && 0);};
void GB::OPCBA5() {assert("Missing" && 0);};
void GB::OPCBA6() {assert("Missing" && 0);};
void GB::OPCBA7() {assert("Missing" && 0);};
void GB::OPCBA8() {assert("Missing" && 0);};
void GB::OPCBA9() {assert("Missing" && 0);};
void GB::OPCBAA() {assert("Missing" && 0);};
void GB::OPCBAB() {assert("Missing" && 0);};
void GB::OPCBAC() {assert("Missing" && 0);};
void GB::OPCBAD() {assert("Missing" && 0);};
void GB::OPCBAE() {assert("Missing" && 0);};
void GB::OPCBAF() {assert("Missing" && 0);};
void GB::OPCBB0() {assert("Missing" && 0);};
void GB::OPCBB1() {assert("Missing" && 0);};
void GB::OPCBB2() {assert("Missing" && 0);};
void GB::OPCBB3() {assert("Missing" && 0);};
void GB::OPCBB4() {assert("Missing" && 0);};
void GB::OPCBB5() {assert("Missing" && 0);};
void GB::OPCBB6() {assert("Missing" && 0);};
void GB::OPCBB7() {assert("Missing" && 0);};
void GB::OPCBB8() {assert("Missing" && 0);};
void GB::OPCBB9() {assert("Missing" && 0);};
void GB::OPCBBA() {assert("Missing" && 0);};
void GB::OPCBBB() {assert("Missing" && 0);};
void GB::OPCBBC() {assert("Missing" && 0);};
void GB::OPCBBD() {assert("Missing" && 0);};
void GB::OPCBBE() {assert("Missing" && 0);};
void GB::OPCBBF() {assert("Missing" && 0);};
void GB::OPCBC0() {assert("Missing" && 0);};
void GB::OPCBC1() {assert("Missing" && 0);};
void GB::OPCBC2() {assert("Missing" && 0);};
void GB::OPCBC3() {assert("Missing" && 0);};
void GB::OPCBC4() {assert("Missing" && 0);};
void GB::OPCBC5() {assert("Missing" && 0);};
void GB::OPCBC6() {assert("Missing" && 0);};
void GB::OPCBC7() {assert("Missing" && 0);};
void GB::OPCBC8() {assert("Missing" && 0);};
void GB::OPCBC9() {assert("Missing" && 0);};
void GB::OPCBCA() {assert("Missing" && 0);};
void GB::OPCBCB() {assert("Missing" && 0);};
void GB::OPCBCC() {assert("Missing" && 0);};
void GB::OPCBCD() {assert("Missing" && 0);};
void GB::OPCBCE() {assert("Missing" && 0);};
void GB::OPCBCF() {assert("Missing" && 0);};
void GB::OPCBD0() {assert("Missing" && 0);};
void GB::OPCBD1() {assert("Missing" && 0);};
void GB::OPCBD2() {assert("Missing" && 0);};
void GB::OPCBD3() {assert("Missing" && 0);};
void GB::OPCBD4() {assert("Missing" && 0);};
void GB::OPCBD5() {assert("Missing" && 0);};
void GB::OPCBD6() {assert("Missing" && 0);};
void GB::OPCBD7() {assert("Missing" && 0);};
void GB::OPCBD8() {assert("Missing" && 0);};
void GB::OPCBD9() {assert("Missing" && 0);};
void GB::OPCBDA() {assert("Missing" && 0);};
void GB::OPCBDB() {assert("Missing" && 0);};
void GB::OPCBDC() {assert("Missing" && 0);};
void GB::OPCBDD() {assert("Missing" && 0);};
void GB::OPCBDE() {assert("Missing" && 0);};
void GB::OPCBDF() {assert("Missing" && 0);};
void GB::OPCBE0() {assert("Missing" && 0);};
void GB::OPCBE1() {assert("Missing" && 0);};
void GB::OPCBE2() {assert("Missing" && 0);};
void GB::OPCBE3() {assert("Missing" && 0);};
void GB::OPCBE4() {assert("Missing" && 0);};
void GB::OPCBE5() {assert("Missing" && 0);};
void GB::OPCBE6() {assert("Missing" && 0);};
void GB::OPCBE7() {assert("Missing" && 0);};
void GB::OPCBE8() {assert("Missing" && 0);};
void GB::OPCBE9() {assert("Missing" && 0);};
void GB::OPCBEA() {assert("Missing" && 0);};
void GB::OPCBEB() {assert("Missing" && 0);};
void GB::OPCBEC() {assert("Missing" && 0);};
void GB::OPCBED() {assert("Missing" && 0);};
void GB::OPCBEE() {assert("Missing" && 0);};
void GB::OPCBEF() {assert("Missing" && 0);};
void GB::OPCBF0() {assert("Missing" && 0);};
void GB::OPCBF1() {assert("Missing" && 0);};
void GB::OPCBF2() {assert("Missing" && 0);};
void GB::OPCBF3() {assert("Missing" && 0);};
void GB::OPCBF4() {assert("Missing" && 0);};
void GB::OPCBF5() {assert("Missing" && 0);};
void GB::OPCBF6() {assert("Missing" && 0);};
void GB::OPCBF7() {assert("Missing" && 0);};
void GB::OPCBF8() {assert("Missing" && 0);};
void GB::OPCBF9() {assert("Missing" && 0);};
void GB::OPCBFA() {assert("Missing" && 0);};
void GB::OPCBFB() {assert("Missing" && 0);};
void GB::OPCBFC() {assert("Missing" && 0);};
void GB::OPCBFD() {assert("Missing" && 0);};
void GB::OPCBFE() {assert("Missing" && 0);};
void GB::OPCBFF() {assert("Missing" && 0);};


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
	std::cout << std::endl;

	std::cout << std::bitset<8>(register8bit[F_REGISTER]) << std::endl;
	std::cout << "ZSHC" << std::endl;

	std::cout << "*******************************************************" << std::endl;
	system("pause");

	std::cout << std::endl;
}

bool GB::createSDLWindow()
{
	//if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
	//{
	//	return false;
	//}

	SDL_Init(SDL_INIT_VIDEO);

	window = SDL_CreateWindow("Gameboy Emulator", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, DISPLAY_WIDTH * 3, DISPLAY_HEIGHT * 3, SDL_WINDOW_ALLOW_HIGHDPI);

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
	bool vBlank = false;

	videoCycles += cycles; //update the cycles (time passed) from the CPU

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
		case H_BLANK:
		{
			vBlank = handleHBlankMode(line);
		}
		break;
		case V_BLANK:
		{
			handleVBlankMode(line, cycles);
		}
		break;
		case OAM:
		{
			handleOAMMode();
		}
		break;
		case LCD_TRANSFER:
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

bool GB::handleHBlankMode(ui8& line)
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

			return true;
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
	videoCycles += cycles;

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
	ui16 tileData = 0;
	ui16 backgroundData = 0;
	ui8 palette = ReadData(BACKGROUND_PALLETTE);
	ui8& control = ReadData(lcdcRegister);
	ui8& scrollY = ReadData(SCROLL_Y);
	std::cout <<  (int)(scrollY) << std::endl;
	if (scrollY == 64)
	{
		system("pause");
	}
	ui8& scrollX = ReadData(SCROLL_X);
	ui8& line = ReadData(LYRegister);
	bool unsig = HasBit(control, 4); //Which tile set to use 1 or 2 (whether the data is signed or unsigned)


	//Fetching Data for tile sets
	if (unsig)
	{
		tileData = 0x8000;
	}
	else
	{
		tileData = 0x8800;
		//unsig = false;
	}

	if (HasBit(control, 3)) //Which background data set 
	{
		backgroundData = 0x9C00;
	}
	else
	{
		backgroundData = 0x9800;
	}

	ui8 xPos = 0;
	ui8 yPos = scrollY + line;

	for (int i = 0; i < DISPLAY_WIDTH; i++)
	{
		xPos = i + scrollX;
		yPos = scrollY + line;
		RenderTile(unsig, backgroundData, tileData, xPos, yPos, i, palette);
	}
}

void GB::RenderWindow(ui8 windowY)
{
	ui16 tileData = 0;
	ui16 backgroundData = 0;
	ui8 palette =ReadData(BACKGROUND_PALLETTE);
	ui8& control = ReadData(lcdcRegister);
	ui8& line = ReadData(LYRegister);
	ui8 windowX = ReadData(WINDOW_X) - 7;

	bool unsig = HasBit(control, 4); //Which tile set to use 1 or 2 (whether the data is signed or unsigned)

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
		backgroundData = 0x9C00;
	}
	else
	{
		backgroundData = 0x9800;
	}

	ui8 xPos = 0;
	ui8 yPos = 0;

	for (int i = 0; i < 160; i++)
	{
		xPos = i - windowX;
		yPos = line - windowY;
		RenderTile(unsig, backgroundData, tileData, xPos, yPos, i, palette);
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
		ui8 index = sprite * 4; //A sprite is 4 bytes
		ui8 yPos = ReadData(0xFE00 + index) - 16;
		ui8 xPos = ReadData(0xFE00 + index + 1) - 8;
		ui8 tileIndex = ReadData(0xFE00 + index + 2);

		//Sprite Attributes
		ui8 attributes = ReadData(0xFE00 + index + 3);

		bool yFlip = HasBit(attributes, 6);
		bool xFlip = HasBit(attributes, 5);

		if ((line >= yPos) && (line < (yPos + spriteHeight)))
		{
			int spriteLine = line - yPos;

			if (yFlip)
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
				int position = tilePixel;
				if (xFlip)
				{
					position -= 7;
					position *= -1;
				}
				ui8 colourNum = (lower >> position) & 1; // Get the set bit
				colourNum <<= 1;
				colourNum |= (upper >> position) & 1;

				if (colourNum != WHITE) // Don't need to draw clear tiles
				{
					int pixelIndex = pixel + DISPLAY_WIDTH * line;
					pixelRGB colour = classicPallette[colourNum];
					//Store them in the framebuffer
					frameBuffer[pixelIndex * 4] = colour.blue;
					frameBuffer[pixelIndex * 4 + 1] = colour.green;
					frameBuffer[pixelIndex * 4 + 2] = colour.red;
				}
			}

		}
	}
}

void GB::RenderTile(bool unsig, ui16 tileMap, ui16 tileData, ui8 xPos, ui8 yPos, ui8 pixel, ui8 pallette)
{
	ui8& line = ReadData(LYRegister);
	// which of the 8 vertical pixels of the current tile is the scanline on?
	ui16 tileRow = (((yPos / 8)) * 32);
	// Out of the 32 horizontal tiles, what one are we currently on
	ui8 tileColumn = (xPos / 8);
	//Get the address for the tileData
	ui16 tileAddress = tileData + tileRow + tileColumn;
	ui16 tileLocation = tileData;
	i16 currentTile;
	//
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
	ui8 vline = yPos % 8 * 2;
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
	pixelRGB colour = classicPallette[colourNum];
	//Store them in the framebuffer
	frameBuffer[pixelIndex * 4] = colour.blue;
	frameBuffer[pixelIndex * 4 + 1] = colour.green;
	frameBuffer[pixelIndex * 4 + 2] = colour.red;
}
