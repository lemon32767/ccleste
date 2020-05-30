HOSTCC?=$(CC)
CFLAGS=-Wall -g -O2

all: celeste-sdl12

celeste-sdl12: sdl12main.c celeste.c celeste.h
	$(CC) $(CFLAGS) sdl12main.c celeste.c -lm `sdl-config --cflags --libs` -lSDL_mixer -o celeste-sdl12

clean:
	$(RM) celeste-sdl12
	make -f Makefile.3ds clean
