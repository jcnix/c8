#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "chip8.h"

void stack_init(struct stack_t *s)
{
	s->next = NULL;
}

void stack_push(struct stack_t *s, int i)
{
	struct stack_t curr;
	curr.i = i;
	curr.next = s;
	s = &curr;
}

int stack_pop(struct stack_t *s)
{
	if(s == NULL)
		return 0;
		
	int i = s->i;
	s = s->next;
	return i;
}

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
	c->delay_timer = 0;
	c->sound_timer = 0;
}

void emulate_cycle(struct chip8 *c)
{
	//fetch 
	c->opcode = c->memory[c->pc] << 8 | c->memory[c->pc+1];
	c->pc += 2;
	int vx = c->opcode & 0x0F00 >> 8;
	int vy = c->opcode & 0x00F0 >> 4;

	//decode opcode
	switch(c->opcode & 0xF000)
	{
		case 0x0000: 
			switch(c->opcode & 0x000F)
			{
				case 0x0000: //0x00E0: clears the screen
					memset(c->gfx, 0, DISP_T);
					c->drawFlag = 1;
					break;
				case 0x000E: //0x00EE: returns from subroutine
					c->pc = stack_pop(&c->stack);
					c->sp -= 1;
					break;
				default:
					printf("Unknown opcode [0x0000]: 0x%X\n", c->opcode);
			}
			break;
		case 0x1000: //1NNN: Jumps to address NNN
			c->pc = c->opcode & 0x0FFF;
			break;
		case 0x2000: //2NNN: Call subroutine at NNN
			stack_push(&c->stack, c->pc);
			c->pc = c->opcode & 0x0FFF;
			break;
		case 0x3000: //3NNN: skip instruction if V[x]= NN
			if(c->V[vx] == c->opcode & 0x00FF)
				c->pc += 2;
			break;
		case 0x4000: //4xNN: skip instruction if V[x] != NN
			if(c->V[vx] != c->opcode & 0x00FF)
				c->pc += 2;
			break;
		case 0x5000: //5xy0: skip instruction if V[x] == V[y]
			if(c->V[vx] == c->V[vy])
				c->pc += 2;
			break;
		case 0x6000: //6xNN: V[x] = NN
			c->V[vx] = c->opcode & 0x00FF;
			break;
		case 0x7000: //7xNN: V[x] += NN
			c->V[vx] += c->opcode & 0x00FF;
			break;
		case 0x8000:
			switch(c->opcode & 0x000F)
			{
				case 0x0000: //8xy0: V[x] = V[y]
					c->V[vx] = c->V[vy];
					break;
				case 0x0001: //8xy1: V[x] = V[x] OR V[y]
					c->V[vx] = c->V[vx] | c->V[vy];
					break;
				case 0x0002: //8xy2: V[x] = V[x] AND V[y]
					c->V[vx] = c->V[vx] & c->V[vy];
					break;
				case 0x0003: //8xy3: V[x] = V[x] XOR V[y]
					c->V[vx] = c->V[vx] ^ c->V[vy];
					break;
				case 0x0004: //8xy4: V[x] += V[y]
					if(c->V[vx] + c->V[vy] > 0xFF)
						c->V[0xF] = 1;
					else
						c->V[0xF] = 0;
					c->V[vx] += c->V[vy];
					c->V[vx] &= 0xFF;
					break;
				case 0x0005: //8xy5: V[x] -= V[y]
					if(c->V[vx] > c->V[vy])
						c->V[0xF] = 1;
					else
						c->V[0xF] = 0;
					c->V[vx] -= c->V[vy];
					c->V[vx] &= 0xFF;
					break;
			}
		case 0xA000: //ANNN sets index register to the address NNN
			c->I = c->opcode & 0x0FFF;
			break;

		default:
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

	for(;;)
	{
		emulate_cycle(&cpu);
		if(cpu.drawFlag)
		{
			//draw graphics
		}

		//set keys
	}

	return 0;
}
