minal:
	gcc -ggdb -o build/minal minal.c -lSDL3 -lSDL3_ttf

run: minal
	./build/minal
