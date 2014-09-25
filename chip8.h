struct chip8 {
	unsigned short opcode;
	unsigned char memory[4096];
	unsigned char V[16];	//registers
	unsigned short I; 		//index register
	unsigned short pc; 		//program counter
	unsigned char gfx[64*32];
	unsigned char drawFlag;
	unsigned char delay_timer;
	unsigned char sound_timer;
	unsigned short stack[16];
	unsigned short sp;		//stack pointer
	unsigned char key[16];	//keypad
};