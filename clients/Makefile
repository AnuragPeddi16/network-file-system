# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Source files
SRC = main.c operations.c utils.c

# Output executable
OUTPUT = a.out

# Build the output executable
$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT) 
	
# Default target
all: $(OUTPUT)