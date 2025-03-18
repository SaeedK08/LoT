## Makefile for Windows

## Location of where main.c is stored in
SRCDIR=./src

## Name of the compiler, GNU GCC in this case
CC=gcc

## Any include directories
INCLUDE = C:\msys64\mingw64\include

## Any flags for the compiler
CFLAGS = -g -c

## For SDL 3
## Note: No -lSDL3main flag needed for SDL3, it is included in -lSDL3 as a header (see main.c file).
## NOTE ORDER OF THE FLAGS MATTERS!
LDFLAGS = -lmingw32 -lSDL3 -lSDL3_net 

## Build the correct main source file according to your SDL version:
main:
	@echo "Building main"
	$(CC) $(CFLAGS) $(SRCDIR)/main.c -o $@.o 
	$(CC) main.o -o main $(LDFLAGS)

clean:
	rm main.exe
	rm main.o