#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <SDL2/sdl.h>
#include "chip8.h"

int opcount = 0;

void stack_push(struct stack_t **head, int i)
{
	struct stack_t *curr = malloc(sizeof(struct stack_t));
	curr->i = i;
	curr->next = *head;
	*head = curr;
	printf("push %d\n", i);
}

int stack_pop(struct stack_t **head)
{
	if(*head == NULL)
		return 0;
		
	int i = (*head)->i;
	struct stack_t *temp = (*head)->next;
	free(*head);
	*head = temp;
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
	c->drawFlag = 0;

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

void _Fx0A(struct chip8 *c, int vx)
{
	char ch = getchar();
	c->V[vx] = ch;
}

void _DxyN(struct chip8 *c, int vx, int vy)
{
	c->V[0xF] = 0;
	int x = c->V[vx] & 0xFF;
	int y = c->V[vy] & 0xFF;
	int height = c->opcode & 0x000F;
	int row = 0;
	while(row < height)
	{
		int curr_row = c->memory[row + c->I];
		int pixel_offset = 0;
		while(pixel_offset < 8)
		{
			int loc = x + pixel_offset + ((y + row) * 64);
			pixel_offset += 1;
			if( ((y + row) >= 32) || ((x + pixel_offset - 1) >= 64) )
			{
				//ignore pixels outside screen
				continue;
			}
			int mask = 1 << (8 - pixel_offset);
			int curr_pixel = (curr_row & mask) >> (8 - pixel_offset);
			c->gfx[loc] ^= curr_pixel;
			if(c->gfx[loc] == 0)
				c->V[0xf] = 1;
			else
				c->V[0xf] = 0;
		}
		row += 1;
	}
	c->drawFlag = 1;
}

void emulate_cycle(struct chip8 *c)
{
	//fetch 
	c->opcode = c->memory[c->pc] << 8 | c->memory[c->pc+1];
	printf("%d: opcode: 0x%04x\n", ++opcount, c->opcode);
	
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
					break;
				default:
					printf("Unknown opcode [0x0000]: 0x%X\n", c->opcode);
					exit(1);
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
			if(c->V[vx] == (c->opcode & 0x00FF))
				c->pc += 2;
			break;
		case 0x4000: //4xNN: skip instruction if V[x] != NN
			if(c->V[vx] != (c->opcode & 0x00FF))
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
			c->V[vx] &= 0xFF;
			break;
		case 0x8000:
			switch(c->opcode & 0x000F)
			{
				case 0x0000: //8xy0: V[x] = V[y]
					c->V[vx] = c->V[vy];
					break;
				case 0x0001: //8xy1: V[x] = V[x] OR V[y]
					c->V[vx] |= c->V[vy];
					break;
				case 0x0002: //8xy2: V[x] = V[x] AND V[y]
					c->V[vx] &= c->V[vy];
					break;
				case 0x0003: //8xy3: V[x] = V[x] XOR V[y]
					c->V[vx] ^= c->V[vy];
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
				case 0x0006: //8xy6: Divide V[x] by 2
					c->V[0xF] = c->V[vx] & 0x0001;
					c->V[vx] = c->V[vx] >> 1;
					break;
				case 0x0007: //8xy7: V[x] = V[y] - V[x]
					if(c->V[vy] > c->V[vx])
						c->V[0xF] = 1;
					else
						c->V[0xF] = 0;
					c->V[vx] = c->V[vy] - c->V[vx];
					c->V[vx] &= 0xFF;
					break;
				case 0x000E: //8xyE: Multiply V[x] by 2
					c->V[0xF] = c->V[vx] & 0x0001;
					c->V[vx] = c->V[vx] << 1;
					c->V[vx] &= 0xFF;
					break;
				default:
					printf("Unknown opcode: [0x8000]: 0x%X\n", c->opcode);
					exit(1);
			}
			break;
		case 0x9000: //9xy0: Skip instruction if V[x] != V[y]
			if(c->V[vx] != c->V[vy])
				c->pc += 2;
			break;
		case 0xA000: //ANNN: set index register to the address NNN
			c->I = c->opcode & 0x0FFF;
			break;
		case 0xB000: //BNNN: Jump to location NNN + V[0]
			c->pc = (c->opcode & 0x0FFF) + c->V[0];
			break;
		case 0xC000: //CxNN: V[x] = rand & NN
			c->V[vx] = (rand() % 0xFF) & (c->opcode & 0x00FF);
			c->V[vx] &= 0xFF;
			break;
		case 0xD000: //Dxyn: Sprite stuff
			_DxyN(c, vx, vy);
			break;
		case 0xE000:
			switch(c->opcode & 0x00FF)
			{
				case 0x009E: //Ex9E: Skip next instruction if key with value of V[x] is pressed
					if(c->keys[c->V[vx] & 0xF])
						c->pc += 2;
					break;
				case 0x00A1: //ExA1: Skip next instruction if key with value of V[x] is not pressed
					if(!(c->keys[c->V[vx] & 0xF]))
						c->pc += 2;
					break;
				default:
					printf("Unknown opcode [0xE000]: 0x%x\n", c->opcode);
					exit(1);
			}
			break;
		case 0xF000:
			switch(c->opcode & 0x00FF)
			{
				case 0x0007: //Fx07: set V[x] to delay timer
					c->V[vx] = c->delay_timer;
					break;
				case 0x000A: //Fx0A: pause and wait for input
					_Fx0A(c, vx);
					break;
				case 0x0015: //Fx15: Set delay timer to V[x]
					c->delay_timer = c->V[vx];
					break;
				case 0x0018: //Fx18: Set sound timer to V[x]
					c->sound_timer = c->V[vx];
					break;
				case 0x001E: //Fx1E: I += V[x]
					c->I += c->V[vx];
					c->V[0xF] = c->I > 0xFFFF;
					break;
				case 0x0029: //Fx29: I = location of sprite for digit Vx
					c->I = (5*(c->V[vx])) & 0xfff;
					break;
				case 0x0033: //Fx33: Store BCD representation of Vx in memory I, I+1, and I+2
					c->memory[c->I] = c->V[vx] / 100;
					c->memory[c->I+1] = (c->V[vx] % 100) / 10;
					c->memory[c->I+2] = c->V[vx] % 10;
					break;
				case 0x0055: //Fx55: Store registers V[0] to V[x] in memory starting at I
					for(int i = 0; i > vx; i++)
					{
						c->memory[c->I+i] = c->V[i];
					}
					c->I += vx + 1;
					break;
				case 0x0065: //Fx65: Read memory from I to I+x and store in V[0] to V[x]
					for(int i = 0; i > vx; i++)
					{
						c->V[i] = c->memory[c->I + i];
					}
					c->I += vx + 1;
					break;
				default:
					printf("Unknown opcode [0xF000]: 0x%x\n", c->opcode);
					exit(1);
			}
			break;
		default:
			printf("unknown opcode 0x%X\n", c->opcode);
			exit(1);
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

int print_stack(struct stack_t *head)
{
	struct stack_t *curr = head;
	while(curr)
	{
		printf("%d ", curr->i);
		curr = curr->next;
	}
	printf("\n");
}

int main(int argc, char** argv)
{
	srand(time(NULL));
	struct chip8 cpu;

	initialize(&cpu);
	load_game(&cpu, argv[1]);

	//setup graphics
	SDL_Window *window = NULL;
	SDL_Surface *screen = NULL;
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf( "SDL could not initialize! SDL_Error: %s\n", SDL_GetError() );
	}
	else
	{
		window = SDL_CreateWindow( "Chip8", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
		if(!window)
		{
			printf( "Window could not be created! SDL_Error: %s\n", SDL_GetError() );
		}
		else
		{
			//Get window surface
            screen = SDL_GetWindowSurface(window);

            //Fill the surface white
            SDL_FillRect( screen, NULL, SDL_MapRGB(screen->format, 0xFF, 0xFF, 0xFF ) );
            
            //Update the surface
            SDL_UpdateWindowSurface(window);
		}
	}

	SDL_Surface *s;
	s = SDL_CreateRGBSurface(0, 640, 320, 32, 0,0,0,0);
	SDL_FillRect(s, NULL, SDL_MapRGB(s->format, 255, 0, 0));
	SDL_Surface *pixel = NULL;
	pixel = SDL_LoadBMP("pixel.bmp");
	if(!pixel)
	{
    	printf( "Unable to load image! SDL Error: %s\n", SDL_GetError() );
	}
	SDL_BlitSurface(s, NULL, screen, NULL);
	SDL_BlitSurface(pixel, NULL, screen, NULL);
	SDL_UpdateWindowSurface(window);

	//setup input
	
	int quit = 0;
	while(!quit)
	{
		emulate_cycle(&cpu);
		if(cpu.drawFlag)
		{
			//draw graphics
		    SDL_BlitSurface(s, NULL, screen, NULL);
		    for(int i = 0; i < DISP_T; i++)
		    {
		    	if(cpu.gfx[i])
		    	{
		    		int x = (i % 64) * 10;
		    		int y = (i / 64) *10;
		    		SDL_Rect r;
		    		r.x = x;
		    		r.y = y;
		    		r.w = 10;
		    		r.h = 10;
		    		SDL_BlitSurface(pixel, NULL, screen, &r);
		    	}
		    }
		    SDL_UpdateWindowSurface(window);
		    cpu.drawFlag = 0;
		}

		//set keys
		SDL_Event e;
		SDL_PollEvent(&e);
		if(e.type == SDL_QUIT)
			quit = 1;
	}

	SDL_FreeSurface(pixel);
	pixel = NULL;

	SDL_DestroyWindow( window );
    SDL_Quit();

	return 0;
}
