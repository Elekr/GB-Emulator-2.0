
#include "Cartridge.h"
#include "GB.h"

int main(int argc, char* argv[])
{
	bool windowOpen = true;
	GB* gameboy = new GB();
	gameboy->createSDLWindow();
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/DrMario.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Tetris.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/01-special.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/03-op sp,hl.gb");
	gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/06-ld r,r.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_instrs.gb");
	gameboy->addBIOS();

	while (windowOpen)
	{
		gameboy->NextFrame();
	}
	return 0;
}