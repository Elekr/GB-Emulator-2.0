
#include "Cartridge.h"
#include "GB.h"
#include <thread>
//https://github.com/ocornut/imgui

short fps = 40; //Feels more natural?
short timePerFrame = 1000 / fps;

int main(int argc, char* argv[])
{
	GB* gameboy = new GB();
	bool windowOpen = gameboy->createSDLWindow();

	if (argc == 2) //Used to access to the emulator through console
	{
		std::cout << "Game loaded: " << argv[1] << std::endl;
		gameboy->InitEMU(argv[1]);
	}
	else
	{
		//(CHANGE STRING TO SELECT GAME)
		gameboy->InitEMU("cpu_instrs.gb"); //Defaults to Blargg's as public use ROM 
	}

	//********* GAMES (Used for testing purposes)	

	//**** ROM Only
	//gameboy->InitEMU("D:/roms/gb/DrMario.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Tetris.gb"); 

	//https://fladnag.net/downloads/telephone/palm/APPS/Liberty1.25/rom2pdb.c Fix for tetris (If interrupts are not implemented)
	//https://www.reddit.com/r/EmuDev/comments/6sxb09/gb_tetris_stuck_at_copyright_screen/ // Tetris bugs intentional

	//**** MBC1
	//gameboy->InitEMU("D:/roms/gb/Super Mario Land.gb");
	//gameboy->InitEMU("D:/roms/gb/Game Boy Gallery.gb");
	gameboy->InitEMU("D:/roms/gb/Kirby's Dream Land.gb");

	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Pokemon Red.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/pocket.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Metroid II.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/LOZ_LA.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Super Mario Land 2.gb"); // (cursed)

	//**** TESTS BLARGGS All Passed
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/01-special.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/02-interrupts.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/03-op sp,hl.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/04-op r,imm.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/05-op rp.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/06-ld r,r.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/07-jr,jp,call,ret,rst.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/08-misc instrs.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/09-op r,r.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/10-bit ops.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/11-op a,(hl).gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/cpu_instrs.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/instr_timing.gb"); 
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/interrupt_time.gb");

	//**** TESTS MOONEYE Some Pass but further development required
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/interrupts/ie_push.gb"); //PASSED
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/instr/daa.gb"); //PASSED
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/timer/tim00_div_trigger.gb"); // PASSED
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/ppu/lcdon_timing-GS.gb"); // FAILS
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/oam_dma/reg_read.gb"); //PASSED

	//**** Official Gameboy Test ROM Passed
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/dmg_test_prog_ver1.gb"); 

	//Used to calculate frame time
	Uint32 startTime = SDL_GetTicks();
	Uint32 endTime = 0;
	Uint32 delta = 0;

	while (windowOpen)
	{
		SDL_Event event; //https://stackoverflow.com/questions/3741055/inputs-in-sdl-on-key-pressed
		while (SDL_PollEvent(&event))
		{
			gameboy->HandleInput(event);
			switch (event.type)
			{
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE)
				{
					windowOpen = false;
				}
				if (event.key.keysym.sym == SDLK_F1)
				{
					gameboy->switchPallete();
				}
			}
			if (event.type == SDL_QUIT)
			{
				windowOpen = false;
			}
		}
		gameboy->Frame();

		endTime = SDL_GetTicks();
		delta = endTime - startTime;

		if (delta < timePerFrame)
		{
			SDL_Delay(timePerFrame - delta);
		}

		startTime = endTime;
	}
	delete gameboy;
	return 0; //Needed by SDL2
}
