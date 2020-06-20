
#include "Cartridge.h"
#include "GB.h"

int main(int argc, char* argv[])
{
	bool windowOpen = true;
	GB* gameboy = new GB();
	gameboy->createSDLWindow();

	//TODO:
	//JOYPAD INPUT
	//TIMINGS
	//MBC
	//DEBUG GAMES

	//**** GAMES
	gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/DrMario.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Tetris.gb"); 
	//https://fladnag.net/downloads/telephone/palm/APPS/Liberty1.25/rom2pdb.c Fix for tetris 
	//https://www.reddit.com/r/EmuDev/comments/6sxb09/gb_tetris_stuck_at_copyright_screen/ // Tetris bugs intentional

	//**** TESTS
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/01-special.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/02-interrupts.gb"); //FAILS
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/03-op sp,hl.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/04-op r,imm.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/05-op rp.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/06-ld r,r.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/07-jr,jp,call,ret,rst.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/08-misc instrs.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/09-op r,r.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/10-bit ops.gb"); // PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/11-op a,(hl).gb"); // PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/cpu_instrs.gb"); // FAILS repeats consistently? timing issue?
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_instrs.gb");
	gameboy->addBIOS();

	while (windowOpen)
	{
		gameboy->NextFrame();
	}
	return 0;
}