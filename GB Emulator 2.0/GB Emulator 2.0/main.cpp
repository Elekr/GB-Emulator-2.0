
#include "Cartridge.h"
#include "GB.h"

int main(int argc, char* argv[])
{
	bool windowOpen = true;
	GB* gameboy = new GB();
	gameboy->createSDLWindow();

	gameboy->InitEMU("C:/Users/Tom/Documents/GB-Emulator-2.0/Games/Tetris.gb");
	//gameboy->InitEMU("C:/Users/Tom\Documents/GB-Emulator-2.0/Games/cpu_individual/01-special.gb");
	gameboy->addBIOS();

	int cycles;

	while (windowOpen)
	{
		gameboy->NextFrame();
		gameboy->RenderGame();
	}

	return 0;
}