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
LDFLAGS = -lmingw32 -lSDL3 -lSDL3_net -lSDL3_image

## List all source files
SOURCES = $(SRCDIR)/main.c $(SRCDIR)/player.c $(SRCDIR)/damage.c $(SRCDIR)/net.c $(SRCDIR)/map.c

## Create object file names from source file names
OBJECTS = $(SOURCES:.c=.o)

## Name of the final executable
EXECUTABLE = main

## Build the executable (main target)
$(EXECUTABLE): $(OBJECTS)
	@echo "Linking..."
	$(CC) $(OBJECTS) -o $(EXECUTABLE) $(LDFLAGS)

## Rule to compile .c files into .o files
%.o: %.c
	@echo "Compiling $<"
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm $(EXECUTABLE).exe $(OBJECTS)