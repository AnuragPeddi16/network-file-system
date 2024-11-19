# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Source files
SRC = main.c client.c

# Output executable
OUTPUT = a.out

# Default target
all: $(OUTPUT)

# Build the output executable
$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT) 

# Clean target to remove the compiled files
clean:
	rm -f $(OUTPUT)

# Run target to execute the compiled program with parameters
run: $(OUTPUT)
	./$(OUTPUT)

.PHONY: all clean run #what does this do??	