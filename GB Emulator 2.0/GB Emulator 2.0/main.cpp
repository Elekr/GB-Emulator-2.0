
#include "Cartridge.h"
#include "GB.h"
#include <chrono> //https://en.cppreference.com/w/cpp/chrono
#include <thread>
//https://github.com/ocornut/imgui

const float FPS = 59.73f;
const float DELAY_TIME = 1000.0f / FPS;

int main(int argc, char* argv[])
{
	GB* gameboy = new GB();
	bool windowOpen = gameboy->createSDLWindow();

	//TODO:
	//JOYPAD INPUT (Test works properly (interrupts)
	//WINDOW RESIZING
	//TIMINGS
	//MBC
	//DEBUG GAMES

	//**** GAMES
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/DrMario.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Tetris.gb"); 
	//https://fladnag.net/downloads/telephone/palm/APPS/Liberty1.25/rom2pdb.c Fix for tetris (If interrupts are not implemented)
	//https://www.reddit.com/r/EmuDev/comments/6sxb09/gb_tetris_stuck_at_copyright_screen/ // Tetris bugs intentional

	//**** TESTS BLARGGS
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/01-special.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/02-interrupts.gb"); // PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/03-op sp,hl.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/04-op r,imm.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/05-op rp.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/06-ld r,r.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/07-jr,jp,call,ret,rst.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/08-misc instrs.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/09-op r,r.gb"); //PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/10-bit ops.gb"); // PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/11-op a,(hl).gb"); // PASSED
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/cpu_individual/cpu_instrs.gb"); // WTF
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/instr_timing.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/interrupt_time.gb");
	//gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Pokemon Red.gb");


	//**** TESTS MOONEYE
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/interrupts/ie_push.gb"); //PASSED
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/instr/daa.gb"); //PASSED
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/timer/tim00_div_trigger.gb"); // PASSED
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/ppu/lcdon_timing-GS.gb"); // FAILS
	//gameboy->InitEMU("C:/users/Tom/Documents/GB-Emulator-2.0/Games/acceptance/oam_dma/reg_read.gb"); //PASSED

	//gameboy->SkipBIOS();

	std::chrono::time_point<std::chrono::high_resolution_clock> current, previous;
	previous = std::chrono::high_resolution_clock::now(); //Time program started

	while (windowOpen)
	{
		current = std::chrono::high_resolution_clock::now(); //Current time
		auto elapsed = std::chrono::duration_cast<std::chrono::duration<float, std::milli>> (current - previous); // Time difference
		previous = current;
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

		if (elapsed.count() < DELAY_TIME) //If the amount of time passed is less than the frame time
		{
			std::this_thread::sleep_for(std::chrono::duration<float, std::milli>(DELAY_TIME - elapsed.count())); //Sleep the thread to make up the difference to keep fps consistent
		}
	}
	return 0;
}
