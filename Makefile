all:
	gcc src/main.c src/chip8.c -O3 -Wall -Wextra `sdl2-config --libs --cflags` -o chip-8

clean:
	rm chip-8
