#pragma once

#include "SDL.h"
#include <iostream>
#include <string>

typedef unsigned __int8 ui8; //8-bit Integer
typedef signed __int8 i8;
typedef unsigned __int16 ui16; //16-bit Integer
typedef signed __int16 i16;

#include "Cartridge.h"

const int A_REGISTER = 0;
const int F_REGISTER = 1;
const int B_REGISTER = 3; //Lower bits
const int C_REGISTER = 2; //Upper bits
const int D_REGISTER = 5;
const int E_REGISTER = 4;
const int H_REGISTER = 7;
const int L_REGISTER = 6;

//16-bit registers
const int AF_REGISTER = 0;
const int BC_REGISTER = 1;
const int DE_REGISTER = 2;
const int HL_REGISTER = 3;
const int PC_REGISTER = 4;
const int SP_REGISTER = 5;

//**** Bits used to control flags in the F register http://marc.rawer.de/Gameboy/Docs/GBCPUman.pdf
const int FLAG_ZERO = 7; // Z
const int FLAG_SUBTRACT = 6; // N
const int FLAG_HALFCARRY = 5; // H
const int FLAG_CARRY = 4; // C

enum CPUInterupt
{
    VBLANK = 0x00,
    LCD = 0x01,
    TIMER = 0x02,
    SERIAL = 0x03,
    JOYPAD = 0x04
};

const ui8 BIOS[256] = {  //NINTENDO BOOT ROM https://realboyemulator.wordpress.com/2013/01/03/a-look-at-the-game-boy-bootstrap-let-the-fun-begin/

	0x31, 0xfe, 0xff, 0xaf, 0x21, 0xff, 0x9f, 0x32, 0xcb, 0x7c, 0x20, 0xfb, 0x21, 0x26, 0xff, 0x0e,
	0x11, 0x3e, 0x80, 0x32, 0xe2, 0x0c, 0x3e, 0xf3, 0xe2, 0x32, 0x3e, 0x77, 0x77, 0x3e, 0xfc, 0xe0,
	0x47, 0x11, 0x04, 0x01, 0x21, 0x10, 0x80, 0x1a, 0xcd, 0x95, 0x00, 0xcd, 0x96, 0x00, 0x13, 0x7b,
	0xfe, 0x34, 0x20, 0xf3, 0x11, 0xd8, 0x00, 0x06, 0x08, 0x1a, 0x13, 0x22, 0x23, 0x05, 0x20, 0xf9,
	0x3e, 0x19, 0xea, 0x10, 0x99, 0x21, 0x2f, 0x99, 0x0e, 0x0c, 0x3d, 0x28, 0x08, 0x32, 0x0d, 0x20,
	0xf9, 0x2e, 0x0f, 0x18, 0xf3, 0x67, 0x3e, 0x64, 0x57, 0xe0, 0x42, 0x3e, 0x91, 0xe0, 0x40, 0x04,
	0x1e, 0x02, 0x0e, 0x0c, 0xf0, 0x44, 0xfe, 0x90, 0x20, 0xfa, 0x0d, 0x20, 0xf7, 0x1d, 0x20, 0xf2,
	0x0e, 0x13, 0x24, 0x7c, 0x1e, 0x83, 0xfe, 0x62, 0x28, 0x06, 0x1e, 0xc1, 0xfe, 0x64, 0x20, 0x06,
	0x7b, 0xe2, 0x0c, 0x3e, 0x87, 0xe2, 0xf0, 0x42, 0x90, 0xe0, 0x42, 0x15, 0x20, 0xd2, 0x05, 0x20,
	0x4f, 0x16, 0x20, 0x18, 0xcb, 0x4f, 0x06, 0x04, 0xc5, 0xcb, 0x11, 0x17, 0xc1, 0xcb, 0x11, 0x17,
	0x05, 0x20, 0xf5, 0x22, 0x23, 0x22, 0x23, 0xc9, 0xce, 0xed, 0x66, 0x66, 0xcc, 0x0d, 0x00, 0x0b,
	0x03, 0x73, 0x00, 0x83, 0x00, 0x0c, 0x00, 0x0d, 0x00, 0x08, 0x11, 0x1f, 0x88, 0x89, 0x00, 0x0e,
	0xdc, 0xcc, 0x6e, 0xe6, 0xdd, 0xdd, 0xd9, 0x99, 0xbb, 0xbb, 0x67, 0x63, 0x6e, 0x0e, 0xec, 0xcc,
	0xdd, 0xdc, 0x99, 0x9f, 0xbb, 0xb9, 0x33, 0x3e, 0x3c, 0x42, 0xb9, 0xa5, 0xb9, 0xa5, 0x42, 0x3c,
	0x21, 0x04, 0x01, 0x11, 0xa8, 0x00, 0x1a, 0x13, 0xbe, 0x20, 0xfe, 0x23, 0x7d, 0xfe, 0x34, 0x20,
	0xf5, 0x06, 0x19, 0x78, 0x86, 0x23, 0x05, 0x20, 0xfb, 0x86, 0x20, 0xfe, 0x3e, 0x01, 0xe0, 0x50,
};

const ui8 normalCycles[256] = {
	1,3,2,2,1,1,2,1,5,2,2,2,1,1,2,1,
	0,3,2,2,1,1,2,1,3,2,2,2,1,1,2,1,
	2,3,2,2,1,1,2,1,2,2,2,2,1,1,2,1,
	2,3,2,2,3,3,3,1,2,2,2,2,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,2,2,2,2,2,0,2,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	1,1,1,1,1,1,2,1,1,1,1,1,1,1,2,1,
	2,3,3,4,3,4,2,4,2,4,3,0,3,6,2,4,
	2,3,3,0,3,4,2,4,2,4,3,0,3,0,2,4,
	3,3,2,0,0,4,2,4,4,1,4,0,0,0,2,4,
	3,3,2,1,0,4,2,4,3,2,4,1,0,0,2,4
};

const ui8 CBCycles[256] = {
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,3,2,2,2,2,2,2,2,3,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2,
	2,2,2,2,2,2,4,2,2,2,2,2,2,2,4,2
};

//**** Timers
//https://github.com/retrio/gb-test-roms/tree/master/instr_timing // Cycle timings

const ui16 m_timer_divider_address = 0xFF04;
const ui16 m_timer_address = 0xFF05;
const ui16 m_timer_modulo_address = 0xFF06;
const ui16 m_timer_controll_address = 0xFF07;



//**************************************************** Display

const int DISPLAY_HEIGHT = 144;
const int DISPLAY_WIDTH = 160;

struct pixelRGB
{
    ui8 red;
    ui8 green;
    ui8 blue;
};

enum colours
{
    WHITE = 0,		//0b11
    LIGHT_GREY = 1, //0b10
    DARK_GREY = 2,  //0b01
    BLACK = 3		//0b00
};

enum GPU_Mode //https://github.com/corybsa/pandocs/blob/develop/content/pixel_fifo.md
{
    H_BLANK = 0,
    V_BLANK = 1,
    OAM = 2,
    LCD_TRANSFER = 3
};

const int VERTICAL_BLANK_SCAN_LINE = 144;
const int VERTICAL_BLANK_SCAN_LINE_MAX = 153;
const int MAX_VIDEO_CYCLES = 456;
const int MIN_HBLANK_MODE_CYCLES = 204;
const int MIN_OAM_MODE_CYCLES = 80;
const int MIN_LCD_TRANSFER_CYCLES = 172;
const int VBLANK_CYCLES = 4560;

const int lcdcRegister = 0xFF40;
const int statusRegister = 0xFF41;
const int LYRegister = 0xFF44;
const int lycRegister = 0xFF45;

const int SCROLL_Y = 0xFF42;
const int SCROLL_X = 0xFF43;

const int WINDOW_Y = 0xFF4A;
const int WINDOW_X = 0xFF4B;

const int BACKGROUND_PALLETTE = 0xFF47;

const ui16 m_cpu_interupt_flag_address = 0xFF0F;
const ui16 m_interrupt_enabled_flag_address = 0xFFFF;

class GB
{
public:
    Cartridge m_cartridge;

    union //creates a union between the 8bit and 16bit registers - (automatically creates 16bit registers eg BC)
    {
        //**** 16-bit registers array
        ui16 register16bit[6]; //AF/BC/DE/HL/PC/SP
        //**** 8-bit registers array
        ui8 register8bit[8]; //A/F/B/C/D/E/H/L
    };
    ui8 m_bus[65536]; //64kb 

    ui8* busPtr = &m_bus[0];
    ui8* dynamicPtr = nullptr;

    ui8 OPCode;

    //**** Interrupts
    bool interruptsEnabled = false;
    bool running = false;
    bool halt = false;

    GB();

    bool InitEMU(const char* path);
    void addBIOS();
    void Reset();

    void WriteData(ui16 address, ui8 data); /////
    ui8& ReadData(ui16 address);/////
    bool InMemoryRange(ui16 start, ui16 end, ui16 address);

    ui8 ReadByte(); /////////
    ui16 ReadWord();/////////

    ui8& GetByteRegister(ui8 reg); //!
    ui16& GetWordRegister(ui8 reg);//!
    //** SET
    void SetByteRegister(ui8 reg, ui8 value); //!
    void SetWordRegister(ui8 reg, ui16 value); //!

    ui8 ReadNextCode(); //!
    void IncrementPC(); //!
    inline void SetPC(const ui16& value); //////

    //**** BITS
    bool HasBit(ui8 data, ui8 bit); //used for the Display potentially can change check flag around 
    void SetBit(ui8 data, ui8 bit);
    void ClearBit(ui8 data, ui8 bit);

    inline void PushStack(ui8 reg);
    inline void PopStack(ui8 reg);
    void PopStackPC(); 

    inline void INCByteRegister(const ui8& reg);
    inline void DECByteRegister(const ui8& reg);

    //**** FLAGS
    void SetFlag(int flag, bool value);////////
    bool CheckFlag(int flag); ////////
    void ClearFlags(); ////////

    //**** JUMPS
    void Jr();

    void ADDHL(const ui16& reg);

    void Bit(const ui8& value, ui8 bit);

    void XOR(const ui8& value);

    void LDI(const ui16 address, const ui8& reg); // https://github.com/jgilchrist/gbemu/blob/master/src/cpu/opcodes.cc

    void LDI(const ui8& reg, const ui16& address);

    void RL(ui8& reg, bool A);

    void RLC(ui8& reg, bool A);

    void RR(ui8& reg, bool A);

    void RRC(ui8& reg, bool A);

    void CP(const ui8& value);

    void SUB(const ui8& value);

    typedef void(GB::*OPCodePtr)(void); // generic function pointer for every code
    OPCodePtr BASECodes[256];
    OPCodePtr CBCodes[256];

    ui16 cycle;
    ui16 cycles;
    int timerCounter;
    int clockFreq;
    unsigned int divCounter;

    void NextFrame();
    bool TickCPU();
    void TickClock();
    void ClockFrequency();

    void InitOPArray();
    void InitCBOPArray();

    //**** DEBUGGING
    void OUTPUTREGISTERS(ui8 op);
    void OUTPUTCBREGISTERS(ui8 op);

    //*************************************************************** Display
    //**** SDL
    SDL_Window* window;
    SDL_Renderer* render;
    SDL_Texture* screen_texture;

    pixelRGB classicPallette[4] = { { 155,188,15 }, { 139,172,15 }, { 48,98,48 }, { 15,56,15 } };
    pixelRGB greyPallette[4] = { { 255,255,255 },{ 0xCC,0xCC,0xCC },{ 0x77,0x77,0x77 }, { 0x0,0x0,0x0 } };

    static const unsigned int m_display_buffer_size = (160 * 144) * 4;

    ui8 frameBuffer[DISPLAY_HEIGHT * DISPLAY_WIDTH * 4]; // viewport region into the frame buffer
    ui8 backBuffer[DISPLAY_HEIGHT * DISPLAY_WIDTH * 4]; // viewport region into the frame buffer

    bool lcdEnabled = false;
    bool vBlank = false;
    GPU_Mode currentMode;
    int modeClock = 0;
    int videoCycles = 0;
    int vBlankCycles = 0;
    int displayEnableDelay = 0;
    int m_oam_pixel = 0;
    int m_oam_tile = 0;


    bool createSDLWindow();

    void DisableLCD();
    void EnableLCD();

    bool updatePixels();
    bool TickDisplay();
    void drawScanline();
    void handleHBlankMode(ui8& line);
    void handleVBlankMode(ui8& line, int cycles);
    void handleOAMMode();
    void handleLCDTransferMode();
    //**** Rendering http://www.codeslinger.co.uk/pages/projects/gameboy/graphics.html
    //https://gbdev.io/pandocs/#video-display
    void RenderGame();
    void RenderBackground();
    void RenderWindow(ui8 windowY);
    void RenderSprites();
    void RenderTile(bool unsig, ui16 tileMap, ui16 tileData, ui8 xPos, ui8 yPos, ui8 pixel, ui8 pallette);

    void RequestInterupt(CPUInterupt interupt); //Handle interupt requests from the Display
    void UpdateLCDStatus();
    void CheckInterrupts();
    void CompareLYWithLYC();

    bool DEBUGGING = false;


//OP CODES
    void OP00(); // NOP
    void OP01(); // LD BC, nn 
    void OP02(); // LD (BC), A
    void OP03(); // INC BC
    void OP04(); // INC B
    void OP05(); // DEC B
    void OP06(); // LD B, ui8
    void OP07(); // RLCA
    void OP08(); // LD (nn) SP
    void OP09(); // ADD HL, BC
    void OP0A(); // LD A, (BC)
    void OP0B(); // DEC BC
    void OP0C(); // INC C
    void OP0D(); // DEC C
    void OP0E(); // LD C, ui8
    void OP0F(); // RRCA
    void OP10(); // STOP
    void OP11(); // LD DE, nn
    void OP12(); // LD (DE), A
    void OP13(); // INC DE
    void OP14(); // INC D
    void OP15(); // DEC D
    void OP16(); // LD D, ui8
    void OP17(); // RLA
    void OP18(); // Jr i8
    void OP19(); // ADD HL, DE
    void OP1A(); // LD A, (DE)
    void OP1B(); // DEC DE
    void OP1C(); // INC E
    void OP1D(); // DEC E
    void OP1E(); // LD E, ui8
    void OP1F(); // RRA
    void OP20(); // JR NZ, i8 
    void OP21(); // LD HL, nn
    void OP22(); // LD (HL+), A
    void OP23(); // INC HL
    void OP24(); // INC H
    void OP25(); // DEC H
    void OP26(); // LD H, ui8
    void OP27(); //
    void OP28(); // JR Z, i8
    void OP29(); // ADD HL, HL
    void OP2A(); // LD A, (HL+)
    void OP2B(); // DEC HL
    void OP2C(); // INC L
    void OP2D(); // DEC L
    void OP2E(); // LD L, ui8
    void OP2F(); // CPL	http://www.cplusplus.com/doc/tutorial/operators/
    void OP30(); // JR NC, i8 (Not Carry)
    void OP31(); // LD SP, nn
    void OP32(); // LD (HL--), A https://blog.tigris.fr/2019/07/28/writing-an-emulator-memory-management/
    void OP33(); // INC SP
    void OP34(); // INC (HL)
    void OP35(); // DEC (HL)
    void OP36(); // LD (HL) ui8
    void OP37(); // SCF http://z80-heaven.wikidot.com/instructions-set:scf
    void OP38(); // JR C, i8
    void OP39(); // ADD HL, SP
    void OP3A(); // LD A, (HL-)
    void OP3B(); // DEC SP
    void OP3C(); // INC A
    void OP3D(); // DEC A
    void OP3E(); // LD A, ui8
    void OP3F(); // CCF http://z80-heaven.wikidot.com/instructions-set:ccf
    void OP40(); // LD B, B
    void OP41(); // LD B, C
    void OP42(); // LD B, D
    void OP43(); // LD B, E
    void OP44(); // LD B, H
    void OP45(); // LD B, L
    void OP46(); // LD B, HL
    void OP47(); // LD B, A
    void OP48(); // LD C, B
    void OP49(); // LD C, C
    void OP4A(); // LD C, D
    void OP4B(); // LD C, E
    void OP4C(); // LD C, H
    void OP4D(); // LD C, L
    void OP4E(); // LD C, HL
    void OP4F(); // LD C, A
    void OP50();
    void OP51();
    void OP52();
    void OP53();
    void OP54();
    void OP55();
    void OP56();
    void OP57(); //LD D, A
    void OP58();
    void OP59();
    void OP5A();
    void OP5B();
    void OP5C();
    void OP5D();
    void OP5E();
    void OP5F(); // LD E, A
    void OP60();
    void OP61();
    void OP62();
    void OP63();
    void OP64();
    void OP65();
    void OP66();
    void OP67(); // LD H, A
    void OP68();
    void OP69();
    void OP6A();
    void OP6B();
    void OP6C();
    void OP6D();
    void OP6E();
    void OP6F(); // LD L, A
    void OP70();
    void OP71();
    void OP72();
    void OP73();
    void OP74();
    void OP75();
    void OP76();
    void OP77(); // LD (HL), A
    void OP78(); // LD A, B
    void OP79();
    void OP7A();
    void OP7B(); // LD A, E
    void OP7C(); // LD A, H
    void OP7D(); // A, L
    void OP7E(); // LD A, (HL)
    void OP7F(); // LD A, A
    void OP80();
    void OP81();
    void OP82();
    void OP83();
    void OP84();
    void OP85();
    void OP86();
    void OP87();
    void OP88();
    void OP89();
    void OP8A();
    void OP8B();
    void OP8C();
    void OP8D();
    void OP8E();
    void OP8F();
    void OP90(); // SUB B
    void OP91();
    void OP92();
    void OP93();
    void OP94();
    void OP95();
    void OP96();
    void OP97();
    void OP98();
    void OP99();
    void OP9A();
    void OP9B();
    void OP9C();
    void OP9D();
    void OP9E();
    void OP9F();
    void OPA0();
    void OPA1();
    void OPA2();
    void OPA3();
    void OPA4();
    void OPA5();
    void OPA6();
    void OPA7();
    void OPA8(); // XOR A, B
    void OPA9(); // XOR A, C
    void OPAA(); // XOR A, D
    void OPAB(); // XOR A, E
    void OPAC(); // XOR A, H
    void OPAD(); // XOR A, L
    void OPAE();
    void OPAF(); // XOR A, A
    void OPB0();
    void OPB1();
    void OPB2();
    void OPB3();
    void OPB4();
    void OPB5();
    void OPB6();
    void OPB7();
    void OPB8();
    void OPB9();
    void OPBA();
    void OPBB();
    void OPBC();
    void OPBD();
    void OPBE(); // CP A, (HL)
    void OPBF();
    void OPC0();
    void OPC1(); // POP BC
    void OPC2();
    void OPC3();
    void OPC4();
    void OPC5(); // PUSH BC
    void OPC6();
    void OPC7();
    void OPC8();
    void OPC9(); // RET
    void OPCA();
    void OPCB();
    void OPCC();
    void OPCD(); // CALL u16 (Push address of next instruction onto stack and then jump to address)
    void OPCE();
    void OPCF();
    void OPD0();
    void OPD1(); // POP DE
    void OPD2();
    void OPD3();
    void OPD4();
    void OPD5(); // PUSH DE
    void OPD6();
    void OPD7();
    void OPD8();
    void OPD9();
    void OPDA();
    void OPDB();
    void OPDC();
    void OPDD();
    void OPDE();
    void OPDF();
    void OPE0(); // LD (FF00+UI8), A
    void OPE1(); // POP HL
    void OPE2(); // LD (FF00 + C), A
    void OPE3();
    void OPE4();
    void OPE5(); // PUSH HL
    void OPE6();
    void OPE7();
    void OPE8();
    void OPE9();
    void OPEA();
    void OPEB();
    void OPEC();
    void OPED();
    void OPEE();
    void OPEF();
    void OPF0();
    void OPF1();
    void OPF2();
    void OPF3();
    void OPF4();
    void OPF5(); // PUSH AF
    void OPF6();
    void OPF7();
    void OPF8();
    void OPF9();
    void OPFA();
    void OPFB(); // EI
    void OPFC();
    void OPFD();
    void OPFE(); // CP, n
    void OPFF();

    //CB OP CODES
    void OPCB00();
    void OPCB01();
    void OPCB02();
    void OPCB03();
    void OPCB04();
    void OPCB05();
    void OPCB06();
    void OPCB07();
    void OPCB08();
    void OPCB09();
    void OPCB0A();
    void OPCB0B();
    void OPCB0C();
    void OPCB0D();
    void OPCB0E();
    void OPCB0F();
    void OPCB10();
    void OPCB11();
    void OPCB12();
    void OPCB13();
    void OPCB14();
    void OPCB15();
    void OPCB16();
    void OPCB17();
    void OPCB18();
    void OPCB19();
    void OPCB1A();
    void OPCB1B();
    void OPCB1C();
    void OPCB1D();
    void OPCB1E();
    void OPCB1F();
    void OPCB20();
    void OPCB21();
    void OPCB22();
    void OPCB23();
    void OPCB24();
    void OPCB25();
    void OPCB26();
    void OPCB27();
    void OPCB28();
    void OPCB29();
    void OPCB2A();
    void OPCB2B();
    void OPCB2C();
    void OPCB2D();
    void OPCB2E();
    void OPCB2F();
    void OPCB30();
    void OPCB31();
    void OPCB32();
    void OPCB33();
    void OPCB34();
    void OPCB35();
    void OPCB36();
    void OPCB37();
    void OPCB38();
    void OPCB39();
    void OPCB3A();
    void OPCB3B();
    void OPCB3C();
    void OPCB3D();
    void OPCB3E();
    void OPCB3F();
    void OPCB40();
    void OPCB41();
    void OPCB42();
    void OPCB43();
    void OPCB44();
    void OPCB45();
    void OPCB46();
    void OPCB47();
    void OPCB48();
    void OPCB49();
    void OPCB4A();
    void OPCB4B();
    void OPCB4C();
    void OPCB4D();
    void OPCB4E();
    void OPCB4F();
    void OPCB50();
    void OPCB51();
    void OPCB52();
    void OPCB53();
    void OPCB54();
    void OPCB55();
    void OPCB56();
    void OPCB57();
    void OPCB58();
    void OPCB59();
    void OPCB5A();
    void OPCB5B();
    void OPCB5C();
    void OPCB5D();
    void OPCB5E();
    void OPCB5F();
    void OPCB60();
    void OPCB61();
    void OPCB62();
    void OPCB63();
    void OPCB64();
    void OPCB65();
    void OPCB66();
    void OPCB67();
    void OPCB68();
    void OPCB69();
    void OPCB6A();
    void OPCB6B();
    void OPCB6C();
    void OPCB6D();
    void OPCB6E();
    void OPCB6F();
    void OPCB70();
    void OPCB71();
    void OPCB72();
    void OPCB73();
    void OPCB74();
    void OPCB75();
    void OPCB76();
    void OPCB77();
    void OPCB78();
    void OPCB79();
    void OPCB7A();
    void OPCB7B();
    void OPCB7C();
    void OPCB7D();
    void OPCB7E();
    void OPCB7F();
    void OPCB80();
    void OPCB81();
    void OPCB82();
    void OPCB83();
    void OPCB84();
    void OPCB85();
    void OPCB86();
    void OPCB87();
    void OPCB88();
    void OPCB89();
    void OPCB8A();
    void OPCB8B();
    void OPCB8C();
    void OPCB8D();
    void OPCB8E();
    void OPCB8F();
    void OPCB90();
    void OPCB91();
    void OPCB92();
    void OPCB93();
    void OPCB94();
    void OPCB95();
    void OPCB96();
    void OPCB97();
    void OPCB98();
    void OPCB99();
    void OPCB9A();
    void OPCB9B();
    void OPCB9C();
    void OPCB9D();
    void OPCB9E();
    void OPCB9F();
    void OPCBA0();
    void OPCBA1();
    void OPCBA2();
    void OPCBA3();
    void OPCBA4();
    void OPCBA5();
    void OPCBA6();
    void OPCBA7();
    void OPCBA8();
    void OPCBA9();
    void OPCBAA();
    void OPCBAB();
    void OPCBAC();
    void OPCBAD();
    void OPCBAE();
    void OPCBAF();
    void OPCBB0();
    void OPCBB1();
    void OPCBB2();
    void OPCBB3();
    void OPCBB4();
    void OPCBB5();
    void OPCBB6();
    void OPCBB7();
    void OPCBB8();
    void OPCBB9();
    void OPCBBA();
    void OPCBBB();
    void OPCBBC();
    void OPCBBD();
    void OPCBBE();
    void OPCBBF();
    void OPCBC0();
    void OPCBC1();
    void OPCBC2();
    void OPCBC3();
    void OPCBC4();
    void OPCBC5();
    void OPCBC6();
    void OPCBC7();
    void OPCBC8();
    void OPCBC9();
    void OPCBCA();
    void OPCBCB();
    void OPCBCC();
    void OPCBCD();
    void OPCBCE();
    void OPCBCF();
    void OPCBD0();
    void OPCBD1();
    void OPCBD2();
    void OPCBD3();
    void OPCBD4();
    void OPCBD5();
    void OPCBD6();
    void OPCBD7();
    void OPCBD8();
    void OPCBD9();
    void OPCBDA();
    void OPCBDB();
    void OPCBDC();
    void OPCBDD();
    void OPCBDE();
    void OPCBDF();
    void OPCBE0();
    void OPCBE1();
    void OPCBE2();
    void OPCBE3();
    void OPCBE4();
    void OPCBE5();
    void OPCBE6();
    void OPCBE7();
    void OPCBE8();
    void OPCBE9();
    void OPCBEA();
    void OPCBEB();
    void OPCBEC();
    void OPCBED();
    void OPCBEE();
    void OPCBEF();
    void OPCBF0();
    void OPCBF1();
    void OPCBF2();
    void OPCBF3();
    void OPCBF4();
    void OPCBF5();
    void OPCBF6();
    void OPCBF7();
    void OPCBF8();
    void OPCBF9();
    void OPCBFA();
    void OPCBFB();
    void OPCBFC();
    void OPCBFD();
    void OPCBFE();
    void OPCBFF();


    std::string OPInstruction[256]
    {
        /* 0x00 */ "SKIP",
        /* 0x01 */ "LD BC, ##",
        /* 0x02 */ "LD (BC), A",
        /* 0x03 */ "INC BC",
        /* 0x04 */ "INC B",
        /* 0x05 */ "DEC B",
        /* 0x06 */ "LD B, #",
        /* 0x07 */ "RLCA",
        /* 0x08 */ "LD (##),SP",
        /* 0x09 */ "ADD HL BC",
        /* 0x0A */ "LD A, (BC)",
        /* 0x0B */ "DEC BC",
        /* 0x0C */ "INC C",
        /* 0x0D */ "DEC C",
        /* 0x0E */ "LD C, #",
        /* 0x0F */ "RRCA",

        /* 0x10 */ "Stop",
        /* 0x11 */ "LD DE, ##",
        /* 0x12 */ "LD (DE), A",
        /* 0x13 */ "INC DE",
        /* 0x14 */ "INC D",
        /* 0x15 */ "DEC D",
        /* 0x16 */ "LD D, #",
        /* 0x17 */ "RLA",
        /* 0x18 */ "JR #",
        /* 0x19 */ "ADD HL, DE",
        /* 0x1A */ "LD A, (DE)",
        /* 0x1B */ "DEC DE",
        /* 0x1C */ "INC E",
        /* 0x1D */ "DEC E",
        /* 0x1E */ "LD E, #",
        /* 0x1F */ "RRA",

        /* 0x20 */ "JR NZ, #",
        /* 0x21 */ "LD HL, nn",
        /* 0x22 */ "LDI (HLI), A",
        /* 0x23 */ "INC HL",
        /* 0x24 */ "INC H",
        /* 0x25 */ "DEC H",
        /* 0x26 */ "LD H, #",
        /* 0x27 */ "DAA",
        /* 0x28 */ "JR Z, #",
        /* 0x29 */ "ADD HL HL",
        /* 0x2A */ "LDI A,(HL)",
        /* 0x2B */ "DEC HL",
        /* 0x2C */ "INC L",
        /* 0x2D */ "DEC L",
        /* 0x2E */ "LD L, #",
        /* 0x2F */ "CPL",

        /* 0x30 */ "JR NC, #",
        /* 0x31 */ "LD SP, ##",
        /* 0x32 */ "LDD (HL--), A",
        /* 0x33 */ "INC SP",
        /* 0x34 */ "INC (HL)",
        /* 0x35 */ "DEC (HL)",
        /* 0x36 */ "LD (HL), #",
        /* 0x37 */ "SCF",
        /* 0x38 */ "JR C, #",
        /* 0x39 */ "ADD HL SP",
        /* 0x3A */ "LD A,(HLD)",
        /* 0x3B */ "DEC SP",
        /* 0x3C */ "INC A",
        /* 0x3D */ "DEC A",
        /* 0x3E */ "LD A, #",
        /* 0x3F */ "CCF",

        /* 0x40 */ "LD B, B",
        /* 0x41 */ "LD B, C",
        /* 0x42 */ "LD B, D",
        /* 0x43 */ "LD B, E",
        /* 0x44 */ "LD B, H",
        /* 0x45 */ "LD B, L",
        /* 0x46 */ "LD B, (HL)",
        /* 0x47 */ "LD B, A",
        /* 0x48 */ "LD C, B",
        /* 0x49 */ "LD C, C",
        /* 0x4A */ "LD C, D",
        /* 0x4B */ "LD C, E",
        /* 0x4C */ "LD C, H",
        /* 0x4D */ "LD C, L",
        /* 0x4E */ "LD C, (HL)",
        /* 0x4F */ "LD C, A",

        /* 0x50 */ "LD D, B",
        /* 0x51 */ "LD D, C",
        /* 0x52 */ "LD D, E",
        /* 0x53 */ "LD D, D",
        /* 0x54 */ "LD D, H",
        /* 0x55 */ "LD D, L",
        /* 0x56 */ "LD D,(HL)",
        /* 0x57 */ "LD D, A",
        /* 0x58 */ "LD E, B",
        /* 0x59 */ "LD E, C",
        /* 0x5A */ "LD E, D",
        /* 0x5B */ "LD E, E",
        /* 0x5C */ "LD E, H",
        /* 0x5D */ "LD E, L",
        /* 0x5E */ "LD E,(HL)",
        /* 0x5F */ "LD E, A",

        /* 0x60 */ "LD H, B",
        /* 0x61 */ "LD H, C",
        /* 0x62 */ "LD H, D",
        /* 0x63 */ "LD H, E",
        /* 0x64 */ "LD H, H",
        /* 0x65 */ "LD H, L",
        /* 0x66 */ "LD H, (HL)",
        /* 0x67 */ "LD H, A",
        /* 0x68 */ "LD L, B",
        /* 0x69 */ "LD L, C",
        /* 0x6A */ "LD L, D",
        /* 0x6B */ "LD L, E",
        /* 0x6C */ "LD L, H",
        /* 0x6D */ "LD L, L",
        /* 0x6E */ "LD L, (HL)",
        /* 0x6F */ "LD L, A",

        /* 0x70 */ "LD (HL), B",
        /* 0x71 */ "LD (HL), C",
        /* 0x72 */ "LD (HL), D",
        /* 0x73 */ "LD (HL), E",
        /* 0x74 */ "LD (HL), H",
        /* 0x75 */ "LD (HL), L",
        /* 0x76 */ "HALT",
        /* 0x77 */ "LD (HL), A",
        /* 0x78 */ "LD A, B",
        /* 0x79 */ "LD A, C",
        /* 0x7A */ "LD A, D",
        /* 0x7B */ "LD A, E",
        /* 0x7C */ "LD A, H",
        /* 0x7D */ "LD A, L",
        /* 0x7E */ "LD A, (HL)",
        /* 0x7F */ "LD A, A",

        /* 0x80 */ "ADD A, B",
        /* 0x81 */ "ADD A, C",
        /* 0x82 */ "ADD A, D",
        /* 0x83 */ "ADD A, E",
        /* 0x84 */ "ADD A, H",
        /* 0x85 */ "ADD A, L",
        /* 0x86 */ "ADD A, (HL)",
        /* 0x87 */ "ADD A, A",
        /* 0x88 */ "ADC B",
        /* 0x89 */ "ADC C",
        /* 0x8A */ "ADC D",
        /* 0x8B */ "ADC E",
        /* 0x8C */ "ADC H",
        /* 0x8D */ "ADC L",
        /* 0x8E */ "ADC (HL)",
        /* 0x8F */ "ADC A",

        /* 0x90 */ "SUB B",
        /* 0x91 */ "SUB C",
        /* 0x92 */ "SUB D",
        /* 0x93 */ "SUB E",
        /* 0x94 */ "SUB H",
        /* 0x95 */ "SUB L",
        /* 0x96 */ "SUB (HL)",
        /* 0x97 */ "SUB A",
        /* 0x98 */ "SBC A, B",
        /* 0x99 */ "SBC A, C",
        /* 0x9A */ "SBC A, D",
        /* 0x9B */ "SBC A, E",
        /* 0x9C */ "SBC A, H",
        /* 0x9D */ "SBC A, L",
        /* 0x9E */ "SBC A, (HL)",
        /* 0x9F */ "SBC A, A",

        /* 0xA0 */ "AND B",
        /* 0xA1 */ "AND C",
        /* 0xA2 */ "AND D",
        /* 0xA3 */ "AND E",
        /* 0xA4 */ "AND H",
        /* 0xA5 */ "AND L",
        /* 0xA6 */ "AND (HL)",
        /* 0xA7 */ "AND A ",
        /* 0xA8 */ "XOR B",
        /* 0xA9 */ "XOR C",
        /* 0xAA */ "XOR D",
        /* 0xAB */ "XOR E",
        /* 0xAC */ "XOR H",
        /* 0xAD */ "XOR L",
        /* 0xAE */ "XOR (HL)",
        /* 0xAF */ "XOR A",

        /* 0xB0 */ "OR B",
        /* 0xB1 */ "OR C",
        /* 0xB2 */ "OR D",
        /* 0xB3 */ "OR E",
        /* 0xB4 */ "OR H",
        /* 0xB5 */ "OR L",
        /* 0xB6 */ "OR (HL)",
        /* 0xB7 */ "OR A",
        /* 0xB8 */ "CP B",
        /* 0xB9 */ "CP C",
        /* 0xBA */ "CP D",
        /* 0xBB */ "CP E",
        /* 0xBC */ "CP H",
        /* 0xBD */ "CP L",
        /* 0xBE */ "CP (HL)",
        /* 0xBF */ "CP A",

        /* 0xC0 */ "RET NZ",
        /* 0xC1 */ "POP BC",
        /* 0xC2 */ "JP NZ, ##",
        /* 0xC3 */ "JP ##",
        /* 0xC4 */ "CALL NZ, ##",
        /* 0xC5 */ "PUSH BC",
        /* 0xC6 */ "ADD A, #",
        /* 0xC7 */ "RESET 00",
        /* 0xC8 */ "RET Z",
        /* 0xC9 */ "RET",
        /* 0xCA */ "JP Z, ##",
        /* 0xCB */ "CB",
        /* 0xCC */ "CALL Z, ##",
        /* 0xCD */ "CALL ##",
        /* 0xCE */ "ADC #",
        /* 0xCF */ "RESET 08",

        /* 0xD0 */ "RET NC",
        /* 0xD1 */ "POP DE",
        /* 0xD2 */ "JP NC, ##",
        /* 0xD3 */ "Invalid",
        /* 0xD4 */ "CALL NC, ##",
        /* 0xD5 */ "PUSH DE",
        /* 0xD6 */ "SUB #",
        /* 0xD7 */ "RESET 10",
        /* 0xD8 */ "RET C",
        /* 0xD9 */ "RETI",
        /* 0xDA */ "JP C, ##",
        /* 0xDB */ "Invalid",
        /* 0xDC */ "CALL C, ##",
        /* 0xDD */ "Invalid",
        /* 0xDE */ "SBC #",
        /* 0xDF */ "RESET 18",

        /* 0xE0 */ "LDH (#), A",
        /* 0xE1 */ "POP HL",
        /* 0xE2 */ "LD (C), A",
        /* 0xE3 */ "Invalid",
        /* 0xE4 */ "Invalid",
        /* 0xE5 */ "PUSH HL",
        /* 0xE6 */ "AND #",
        /* 0xE7 */ "RESET 20",
        /* 0xE8 */ "ADD SP, #",
        /* 0xE9 */ "JP HL",
        /* 0xEA */ "LD (##), A",
        /* 0xEB */ "Invalid",
        /* 0xEC */ "Invalid",
        /* 0xED */ "Invalid",
        /* 0xEE */ "XOR *",
        /* 0xEF */ "RESET 28",

        /* 0xF0 */ "LDH A, 0xFF00 + (#)",
        /* 0xF1 */ "POP AF",
        /* 0xF2 */ "LD A, FF00 + (C)",
        /* 0xF3 */ "DI",
        /* 0xF4 */ "Invalid",
        /* 0xF5 */ "PUSH AF",
        /* 0xF6 */ "OR #",
        /* 0xF7 */ "RESET 30",
        /* 0xF8 */ "LDHL SP, #",
        /* 0xF9 */ "LD SP, HL",
        /* 0xFA */ "LD A, (##)",
        /* 0xFB */ "EI",
        /* 0xFC */ "INVALID",
        /* 0xFD */ "INVALID",
        /* 0xFE */ "CP #",
        /* 0xFF */ "RESET 38",
    };

    std::string CBOPInstruction[256]
    {
        /* CB 0x00 */ "RLC B",
        /* CB 0x01 */ "RLC C",
        /* CB 0x02 */ "RLC D",
        /* CB 0x03 */ "RLC E",
        /* CB 0x04 */ "RLC H",
        /* CB 0x05 */ "RLC L",
        /* CB 0x06 */ "RLC (HL)",
        /* CB 0x07 */ "RLC A",
        /* CB 0x08 */ "RRC B",
        /* CB 0x09 */ "RRC C",
        /* CB 0x0A */ "RRC D",
        /* CB 0x0B */ "RRC E",
        /* CB 0x0C */ "RRC H",
        /* CB 0x0D */ "RRC L",
        /* CB 0x0E */ "RRC (HL)",
        /* CB 0x0F */ "RRC A",

        /* CB 0x10 */ "RL B",
        /* CB 0x11 */ "RL C",
        /* CB 0x12 */ "RL D",
        /* CB 0x13 */ "RL E",
        /* CB 0x14 */ "RL H",
        /* CB 0x15 */ "RL L",
        /* CB 0x16 */ "RL (HL)",
        /* CB 0x17 */ "RL A",
        /* CB 0x18 */ "RR B",
        /* CB 0x19 */ "RR C",
        /* CB 0x1A */ "RR D",
        /* CB 0x1B */ "RR E",
        /* CB 0x1C */ "RR H",
        /* CB 0x1D */ "RR L",
        /* CB 0x1E */ "RR (HL)",
        /* CB 0x1F */ "RR A",

        /* CB 0x20 */ "SLA B",
        /* CB 0x21 */ "SLA C",
        /* CB 0x22 */ "SLA D",
        /* CB 0x23 */ "SLA E",
        /* CB 0x24 */ "SLA H",
        /* CB 0x25 */ "SLA L",
        /* CB 0x26 */ "SLA (HL)",
        /* CB 0x27 */ "SLA A",
        /* CB 0x28 */ "SRA B",
        /* CB 0x29 */ "SRA C",
        /* CB 0x2A */ "SRA D",
        /* CB 0x2B */ "SRA E",
        /* CB 0x2C */ "SRA H",
        /* CB 0x2D */ "SRA L",
        /* CB 0x2E */ "SRA (HL)",
        /* CB 0x2F */ "SRA A",

        /* CB 0x30 */ "SWAP B",
        /* CB 0x31 */ "SWAP C",
        /* CB 0x32 */ "SWAP D",
        /* CB 0x33 */ "SWAP E",
        /* CB 0x34 */ "SWAP H",
        /* CB 0x35 */ "SWAP L",
        /* CB 0x36 */ "SWAP (HL)",
        /* CB 0x37 */ "SWAP A",
        /* CB 0x38 */ "SRL B",
        /* CB 0x39 */ "SRL C",
        /* CB 0x3A */ "SRL D",
        /* CB 0x3B */ "SRL E",
        /* CB 0x3C */ "SRL H",
        /* CB 0x3D */ "SRL L",
        /* CB 0x3E */ "SRL (HL)",
        /* CB 0x3F */ "SRL A",

        /* CB 0x40 */ "BIT 0, B",
        /* CB 0x41 */ "BIT 0, C",
        /* CB 0x42 */ "BIT 0, D",
        /* CB 0x43 */ "BIT 0, E",
        /* CB 0x44 */ "BIT 0, H",
        /* CB 0x45 */ "BIT 0, L",
        /* CB 0x46 */ "BIT 0, (HL)",
        /* CB 0x47 */ "BIT 0, A",
        /* CB 0x48 */ "BIT 1, B",
        /* CB 0x49 */ "BIT 1, C",
        /* CB 0x4A */ "BIT 1, D",
        /* CB 0x4B */ "BIT 1, E",
        /* CB 0x4C */ "BIT 1, H",
        /* CB 0x4D */ "BIT 1, L",
        /* CB 0x4E */ "BIT 1, (HL)",
        /* CB 0x4F */ "BIT 1, A",

        /* CB 0x50 */ "BIT 2, B",
        /* CB 0x51 */ "BIT 2, C",
        /* CB 0x52 */ "BIT 2, D",
        /* CB 0x53 */ "BIT 2, E",
        /* CB 0x54 */ "BIT 2, H",
        /* CB 0x55 */ "BIT 2, L",
        /* CB 0x56 */ "BIT 2, (HL)",
        /* CB 0x57 */ "BIT 2, A",
        /* CB 0x58 */ "BIT 3, B",
        /* CB 0x59 */ "BIT 3, C",
        /* CB 0x5A */ "BIT 3, D",
        /* CB 0x5B */ "BIT 3, E",
        /* CB 0x5C */ "BIT 3, H",
        /* CB 0x5D */ "BIT 3, L",
        /* CB 0x5E */ "BIT 3, (HL)",
        /* CB 0x5F */ "BIT 3, A",

        /* CB 0x60 */ "BIT 4, B",
        /* CB 0x61 */ "BIT 4, C",
        /* CB 0x62 */ "BIT 4, D",
        /* CB 0x63 */ "BIT 4, E",
        /* CB 0x64 */ "BIT 4, H",
        /* CB 0x65 */ "BIT 4, L",
        /* CB 0x66 */ "BIT 4, (HL)",
        /* CB 0x67 */ "BIT 4, A",
        /* CB 0x68 */ "BIT 5, B",
        /* CB 0x69 */ "BIT 5, C",
        /* CB 0x6A */ "BIT 5, D",
        /* CB 0x6B */ "BIT 5, E",
        /* CB 0x6C */ "BIT 5, H",
        /* CB 0x6D */ "BIT 5, L",
        /* CB 0x6E */ "BIT 5, (HL)",
        /* CB 0x6F */ "BIT 5, A",

        /* CB 0x70 */ "BIT 6, B",
        /* CB 0x71 */ "BIT 6, C",
        /* CB 0x72 */ "BIT 6, D",
        /* CB 0x73 */ "BIT 6, E",
        /* CB 0x74 */ "BIT 6, H",
        /* CB 0x75 */ "BIT 6, L",
        /* CB 0x76 */ "BIT 6, (HL)",
        /* CB 0x77 */ "BIT 6, A",
        /* CB 0x78 */ "BIT 7, B",
        /* CB 0x79 */ "BIT 7, C",
        /* CB 0x7A */ "BIT 7, D",
        /* CB 0x7B */ "BIT 7, E",
        /* CB 0x7C */ "BIT 7, H",
        /* CB 0x7D */ "BIT 7, L",
        /* CB 0x7E */ "BIT 7, (HL)",
        /* CB 0x7F */ "BIT 7, A",

        /* CB 0x80 */ "RES 0, B",
        /* CB 0x81 */ "RES 0, C",
        /* CB 0x82 */ "RES 0, D",
        /* CB 0x83 */ "RES 0, E",
        /* CB 0x84 */ "RES 0, H",
        /* CB 0x85 */ "RES 0, L",
        /* CB 0x86 */ "RES 0,(HL)",
        /* CB 0x87 */ "RES 0, A",
        /* CB 0x88 */ "RES 1, B",
        /* CB 0x89 */ "RES 1, C",
        /* CB 0x8A */ "RES 1, D",
        /* CB 0x8B */ "RES 1, E",
        /* CB 0x8C */ "RES 1, H",
        /* CB 0x8D */ "RES 1, L",
        /* CB 0x8E */ "RES 1,(HL)",
        /* CB 0x8F */ "RES 1, A",

        /* CB 0x90 */ "RES 2, B",
        /* CB 0x91 */ "RES 2, C",
        /* CB 0x92 */ "RES 2, D",
        /* CB 0x93 */ "RES 2, E",
        /* CB 0x94 */ "RES 2, H",
        /* CB 0x95 */ "RES 2, L",
        /* CB 0x96 */ "RES 2,(HL)",
        /* CB 0x97 */ "RES 2, A",
        /* CB 0x98 */ "RES 3, B",
        /* CB 0x99 */ "RES 3, C",
        /* CB 0x9A */ "RES 3, D",
        /* CB 0x9B */ "RES 3, E",
        /* CB 0x9C */ "RES 3, H",
        /* CB 0x9D */ "RES 3, L",
        /* CB 0x9E */ "RES 3,(HL)",
        /* CB 0x9F */ "RES 3, A",

        /* CB 0xA0 */ "RES 4, B",
        /* CB 0xA1 */ "RES 4, C",
        /* CB 0xA2 */ "RES 4, D",
        /* CB 0xA3 */ "RES 4, E",
        /* CB 0xA4 */ "RES 4, H",
        /* CB 0xA5 */ "RES 4, L",
        /* CB 0xA6 */ "RES 4,(HL)",
        /* CB 0xA7 */ "RES 4, A",
        /* CB 0xA8 */ "RES 5, B",
        /* CB 0xA9 */ "RES 5, C",
        /* CB 0xAA */ "RES 5, D",
        /* CB 0xAB */ "RES 5, E",
        /* CB 0xAC */ "RES 5, H",
        /* CB 0xAD */ "RES 5, L",
        /* CB 0xAE */ "RES 5,(HL)",
        /* CB 0xAF */ "RES 5, A",

        /* CB 0xB0 */ "RES 6, B",
        /* CB 0xB1 */ "RES 6, C",
        /* CB 0xB2 */ "RES 6, D",
        /* CB 0xB3 */ "RES 6, E",
        /* CB 0xB4 */ "RES 6, H",
        /* CB 0xB5 */ "RES 6, L",
        /* CB 0xB6 */ "RES 6,(HL)",
        /* CB 0xB7 */ "RES 6, A",
        /* CB 0xB8 */ "RES 7, B",
        /* CB 0xB9 */ "RES 7, C",
        /* CB 0xBA */ "RES 7, D",
        /* CB 0xBB */ "RES 7, E",
        /* CB 0xBC */ "RES 7, H",
        /* CB 0xBD */ "RES 7, L",
        /* CB 0xBE */ "RES 7,(HL)",
        /* CB 0xBF */ "RES 7, A",

        /* CB 0xC0 */ "SET 0, B",
        /* CB 0xC1 */ "SET 0, C",
        /* CB 0xC2 */ "SET 0, D",
        /* CB 0xC3 */ "SET 0, E",
        /* CB 0xC4 */ "SET 0, H",
        /* CB 0xC5 */ "SET 0, L",
        /* CB 0xC6 */ "SET 0,(HL)",
        /* CB 0xC7 */ "SET 0, A",
        /* CB 0xC8 */ "SET 1, B",
        /* CB 0xC9 */ "SET 1, C",
        /* CB 0xCA */ "SET 1, D",
        /* CB 0xCB */ "SET 1, E",
        /* CB 0xCC */ "SET 1, H",
        /* CB 0xCD */ "SET 1, L",
        /* CB 0xCE */ "SET 1,(HL)",
        /* CB 0xCF */ "SET 1, A",

        /* CB 0xD0 */ "SET 2, B",
        /* CB 0xD1 */ "SET 2, C",
        /* CB 0xD2 */ "SET 2, D",
        /* CB 0xD3 */ "SET 2, E",
        /* CB 0xD4 */ "SET 2, H",
        /* CB 0xD5 */ "SET 2, L",
        /* CB 0xD6 */ "SET 2,(HL)",
        /* CB 0xD7 */ "SET 2, A",
        /* CB 0xD8 */ "SET 3, B",
        /* CB 0xD9 */ "SET 3, C",
        /* CB 0xDA */ "SET 3, D",
        /* CB 0xDB */ "SET 3, E",
        /* CB 0xDC */ "SET 3, H",
        /* CB 0xDD */ "SET 3, L",
        /* CB 0xDE */ "SET 3,(HL)",
        /* CB 0xDF */ "SET 3, A",

        /* CB 0xE0 */ "SET 4, B",
        /* CB 0xE1 */ "SET 4, C",
        /* CB 0xE2 */ "SET 4, D",
        /* CB 0xE3 */ "SET 4, E",
        /* CB 0xE4 */ "SET 4, H",
        /* CB 0xE5 */ "SET 4, L",
        /* CB 0xE6 */ "SET 4,(HL)",
        /* CB 0xE7 */ "SET 4, A",
        /* CB 0xE8 */ "SET 5, B",
        /* CB 0xE9 */ "SET 5, C",
        /* CB 0xEA */ "SET 5, D",
        /* CB 0xEB */ "SET 5, E",
        /* CB 0xEC */ "SET 5, H",
        /* CB 0xED */ "SET 5, L",
        /* CB 0xEE */ "SET 5,(HL)",
        /* CB 0xEF */ "SET 5, A",

        /* CB 0xF0 */ "SET 6, B",
        /* CB 0xF1 */ "SET 6, C",
        /* CB 0xF2 */ "SET 6, D",
        /* CB 0xF3 */ "SET 6, E",
        /* CB 0xF4 */ "SET 6, H",
        /* CB 0xF5 */ "SET 6, L",
        /* CB 0xF6 */ "SET 6,(HL)",
        /* CB 0xF7 */ "SET 6, A",
        /* CB 0xF8 */ "SET 7, B",
        /* CB 0xF9 */ "SET 7, C",
        /* CB 0xFA */ "SET 7, D",
        /* CB 0xFB */ "SET 7, E",
        /* CB 0xFC */ "SET 7, H",
        /* CB 0xFD */ "SET 7, L",
        /* CB 0xFE */ "SET 7,(HL)",
        /* CB 0xFF */ "SET 7, A",
    };


};
