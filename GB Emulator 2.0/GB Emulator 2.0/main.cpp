
#include "Cartridge.h"
#include "GB.h"

int main(int argc, char* argv[])
{
	bool windowOpen = true;
	GB* gameboy = new GB();
	gameboy->createSDLWindow();
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/DrMario.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Tetris.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/01-special.gb"); //PASSED
	gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/02-interrupts.gb"); //FAILS
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/03-op sp,hl.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/04-op r,imm.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/05-op rp.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/06-ld r,r.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/07-jr,jp,call,ret,rst.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/08-misc instrs.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/09-op r,r.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/10-bit ops.gb"); // PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/11-op a,(hl).gb"); // PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/cpu_instrs.gb"); // FAILS EI to blame?
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_instrs.gb");
	gameboy->addBIOS();

	while (windowOpen)
	{
		gameboy->NextFrame();
	}
	return 0;
}