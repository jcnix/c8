OBJS = main.c

OBJ_NAME = main

SDL2 = -I/usr/local/include/SDL2 -D_THREAD_SAFE -L/usr/local/lib -lSDL2

all: $(OBJS)
	cc $(OBJS) -w $(SDL2) -o $(OBJ_NAME)
