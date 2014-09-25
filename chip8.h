#define DISP_T 64*32 //Display buffer size

struct stack_t {
	unsigned short i;
	struct stack_t *next;
};

struct chip8 {
	unsigned short opcode;
	unsigned char memory[4096];
	unsigned char V[16];	//registers
	unsigned short I; 		//index register
	unsigned short pc; 		//program counter
	unsigned char gfx[DISP_T];
	unsigned char drawFlag;
	unsigned char delay_timer;
	unsigned char sound_timer;
	struct stack_t stack;
	unsigned short sp;		//stack pointer
	unsigned char keys[16];	//keypad
};