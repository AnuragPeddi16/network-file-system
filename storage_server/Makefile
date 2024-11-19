# Compiler
CC = gcc

# Compiler flags
CFLAGS = -Wall -g

# Source files
SRC = main.c operations.c utils.c

# Output executable
OUTPUT = storage_server

# Default target
all: $(OUTPUT)

# Build the output executable
$(OUTPUT): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(OUTPUT) -D NM_IP=$(nm_ip) -D ACCESSIBLE_PATHS=$(accessible_paths)

# Clean target to remove the compiled files
clean:
	rm -f $(OUTPUT)

# Run target to execute the compiled program with parameters
run: $(OUTPUT)
	./$(OUTPUT) $(nm_ip) $(accessible_paths)

.PHONY: all clean run