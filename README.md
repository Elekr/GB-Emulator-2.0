# GB-Emulator-2.0

The project was submitted as part of my final year double project and was given the grade 85/100

The project itself is a Gameboy Emulator capable of running original Gameboy software (ROM Only & MBC1 Cartridges) with minor glitches.
The emulator was developed using C++ with SDL2 library to output the display to the screen.

How to use:

The emulator uses SDL2 which can be found at https://www.libsdl.org/
The default test for the Emulator is the blargs "cpu_instrs.gb" test found at https://github.com/retrio/gb-test-roms/tree/master/cpu_instrs

1)If using Visual Studio 2019 change the line "gameboy->InitEMU("cpu_instrs.gb");" by replacing the "cpu_instrs" with the ROM file name
2a)If using Command prompt, change the directory to the location of the emulator. For example, "C:\Users\Tom\Documents\GB Emulator 2.0
2b)Start the emulator by entering "start GB Emulator 2.0.exe gamename.gb" replacing gamename with the name of the ROM file.


A demonstration of the project can be found at: https://www.youtube.com/watch?v=AVyAk8o_ux0
