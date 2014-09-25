#include <stdio.h>
#include <stdlib.h>
#include "chip8.h"

void load_game(struct chip8 *c, char* game)
{
	printf("game: %s\n", game);
	FILE *fp = fopen(game, "rb");
	if(fp == NULL)
	{
		printf("File error\n");
		exit(1);
	}

	fseek(fp, 0L, SEEK_END);
	int bufferSize = ftell(fp);
	rewind(fp);
	printf("bufferSize: %d\n", bufferSize);

	fread(&c->memory[0x200], 1, bufferSize, fp);

	fclose(fp);
}

void initialize(struct chip8 *c)
{
	c->pc = 0x200;
	c->opcode = 0;
	c->I = 0;
	c->sp = 0;

	unsigned char fontset[80] = { 
	  0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
	  0x20, 0x60, 0x20, 0x20, 0x70, // 1
	  0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
	  0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
	  0x90, 0x90, 0xF0, 0x10, 0x10, // 4
	  0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
	  0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
	  0xF0, 0x10, 0x20, 0x40, 0x40, // 7
	  0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
	  0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
	  0xF0, 0x90, 0xF0, 0x90, 0x90, // A
	  0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
	  0xF0, 0x80, 0x80, 0x80, 0xF0, // C
	  0xE0, 0x90, 0x90, 0x90, 0xE0, // D
	  0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
	  0xF0, 0x80, 0xF0, 0x80, 0x80  // F
	};

	//clear display
	//clear stack
	//clear registers V0-VF
	//clear memory

	//Load fontset
	int i = 0;
	for(i =0; i < 80; ++i)
	{
		c->memory[i] = fontset[i];
	}

	//reset timers
}

void emulate_cycle(struct chip8 *c)
{
	//fetch 
	c->opcode = c->memory[c->pc] << 8 | c->memory[c->pc+1];
	c->pc += 2;

	//decode opcode
	switch(c->opcode & 0xF000)
	{
		case 0x1000: //Jumps to address NNN
			c->pc = c->opcode & 0x0FFF;
			break;
		//execute opcode
		case 0xA000: //ANNN sets index register to the address NNN
			c->I = c->opcode & 0x0FFF;
			break;

		default: //unknown opcode
			printf("unknown opcode 0x%X\n", c->opcode);
	}

	//update timers
	if(c->delay_timer > 0)
		--c->delay_timer;

	if(c->sound_timer > 0)
	{
		if(c->sound_timer == 1)
			printf("BEEP\n");
		--c->sound_timer;
	}
}


int main(int argc, char** argv)
{
	struct chip8 cpu;

	initialize(&cpu);
	load_game(&cpu, argv[1]);

	//setup graphics
	//setup input

	/*for(;;)
	{
		emulate_cycle(cpu);
		if(cpu->drawFlag)
		{
			//draw graphics
		}

		//set keys
	}*/

	return 0;
}
