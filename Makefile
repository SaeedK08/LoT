SRCDIR := src
OBJDIR := obj
BINDIR := bin

## Compiler
CC := clang
INCLUDE := /opt/homebrew/include
CFLAGS := -g -I$(INCLUDE) -Wall

LIBS := /opt/homebrew/lib
LDFLAGS := -lSDL3 -lSDL3_net -lSDL3_image -lSDL3_ttf

## Find all .c files and generate .o file names
SRC := $(wildcard $(SRCDIR)/*.c)
OBJ := $(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SRC))

## Target binary name
TARGET := $(BINDIR)/main

## Default build
all: $(TARGET)

## Link object files to final binary
$(TARGET): $(OBJ)
	@mkdir -p $(BINDIR)
	$(CC) $^ -o $@ -L$(LIBS) $(LDFLAGS)

## Compile each .c to .o
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

## Run the game
run: all
	./$(TARGET) $(ARGS)

## Clean up
clean:
	rm -rf $(OBJDIR) $(BINDIR)
