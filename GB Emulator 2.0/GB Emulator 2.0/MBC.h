#pragma once

typedef unsigned __int8 ui8; //8-bit Integer
typedef signed __int8 i8;
typedef unsigned __int16 ui16; //16-bit Integer
typedef signed __int16 i16;

class Cartridge;
class MBC
{
public:

	virtual void write(ui16 address, ui8 data) = 0;

	MBC(Cartridge* cart, ui8* bus) : m_cart(cart), m_bus(bus)
	{

	}

protected:
	Cartridge* m_cart;
	ui8* m_bus;
};

