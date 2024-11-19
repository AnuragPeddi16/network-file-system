#include "client.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAX_ARGS 4
#define MAX_INPUT_SIZE 1024
#define PROMPT "nfs> "

// Helper function to trim whitespace from a string
char* trim(char* str) {
    char* end;
    
    // Trim leading whitespace
    while(isspace((unsigned char)*str)) str++;
    
    if(*str == 0) return str;  // All spaces
    
    // Trim trailing whitespace
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    
    end[1] = '\0';
    return str;
}

// Parse command line into arguments
int parse_command(char* line, char** args) {
    int argc = 0;
    // char* token;
    char* rest = line;
    int in_quotes = 0;
    char* quote_start = NULL;
    
    // Handle quoted strings and regular arguments
    while (*rest && argc < MAX_ARGS) {
        // Skip leading spaces
        while (*rest && isspace(*rest)) rest++;
        
        if (!*rest) break;
        
        if (*rest == '"') {
            if (!in_quotes) {
                // Start of quoted string
                in_quotes = 1;
                quote_start = rest + 1;
            } else {
                // End of quoted string
                *rest = '\0';
                args[argc++] = quote_start;
                in_quotes = 0;
            }
        } else if (!in_quotes && !isspace(*rest)) {
            // Start of regular argument
            args[argc++] = rest;
            // Find end of argument
            while (*rest && !isspace(*rest) && *rest != '"') rest++;
            if (*rest) {
                *rest = '\0';
                rest++;
            }
            continue;
        }
        
        rest++;
    }
    
    return argc;
}

void print_help() {
    printf("\nNFS Client Commands:\n");
    printf("------------------\n");
    printf("READ <path>\n");
    printf("    Read contents of a file\n");
    printf("WRITE <path> \"<content>\"\n");
    printf("    Write content to a file\n");
    printf("CREATE <path> <name>\n");
    printf("    Create a new file or directory\n");
    printf("DELETE <path>\n");
    printf("    Delete a file or directory\n");
    printf("LIST <path>\n");
    printf("    List contents of a directory\n");
    printf("INFO <path>\n");
    printf("    Show file/directory information\n");
    printf("STREAM <path>\n");
    printf("    Stream an audio file\n");
    printf("COPY <source> <dest>\n");
    printf("    Copy a file or directory\n");
    printf("HELP\n");
    printf("    Display this help message\n");
    printf("EXIT\n");
    printf("    Exit the NFS client\n\n");
}

void execute_command(int argc, char** args) {
    if (argc == 0) return;

    // Convert command to uppercase for case-insensitive comparison
    char cmd[32];
    strncpy(cmd, args[0], sizeof(cmd) - 1);
    cmd[sizeof(cmd) - 1] = '\0';
    for (int i = 0; cmd[i]; i++) {
        cmd[i] = toupper(cmd[i]);
    }
    printf("Command: %s\n", cmd);

    if (strcmp(cmd, "HELP") == 0) {
        print_help();
    }
    else if (strcmp(cmd, "READ") == 0) {
        if (argc != 2) {
            printf("Usage: READ <path>\n");
            return;
        }
        if (handle_read_operation(args[1]) == 0) {
            printf("\nRead operation completed successfully.\n");
        }
    }
    else if (strcmp(cmd, "WRITE") == 0) {
        if (argc < 3 || argc > 4 ) { //3 or 4
            printf("Usage: WRITE <path> \"<content>\"\n");
            return;
        }
        // if 4 then concatenate 3 and 4
        if (argc == 4) {
            strcat(args[2], " ");
            strcat(args[2], args[3]);
        }
        if (handle_write_operation(args[1], args[2]) == 0) {
            printf("Write operation completed successfully.\n");
        }
    }
    else if (strcmp(cmd, "CREATE") == 0) {
        if (argc != 3) {
            printf("Usage: CREATE <path> <name>\n");
            return;
        }
        if (handle_create_operation(args[1], args[2]) == 0) {
            printf("Create operation completed successfully.\n");
        }
    }
    else if (strcmp(cmd, "DELETE") == 0) {
        if (argc != 2) {
            printf("Usage: DELETE <path>\n");
            return;
        }
        if (handle_delete_operation(args[1]) == 0) {
            printf("Delete operation completed successfully.\n");
        }
    }
    else if (strcmp(cmd, "LIST") == 0) {
        if (argc != 1) {
            printf("Usage: LIST \n");
            return;
        }
        if (handle_list_operation()==0) {
            printf("List operation completed successfully.\n");
        }
    }
    else if (strcmp(cmd, "STREAM") == 0) {
        if (argc != 2) {
            printf("Usage: STREAM <path>\n");
            return;
        }
        if (handle_stream_operation(args[1]) == 0) {
            printf("Stream operation completed successfully.\n");
        }
    }
    else if (strcmp(cmd, "COPY") == 0) {
        if (argc != 3) {
            printf("Usage: COPY <source> <dest>\n");
            
            return;
        }
        if (handle_copy_operation(args[1], args[2]) == 0) {
            printf("Copy operation completed successfully.\n");
        }
    }
    else if (strcmp(cmd, "INFO") == 0){
        if (argc != 2) {
            printf("Usage: INFO <path>\n");
            return;
        }
        if (handle_info_operation(args[1]) == 0) {
            printf("INFO operation completed successfully.\n");
        }
    }
    
    else if (strcmp(cmd, "EXIT") == 0) {
        printf("Goodbye!\n");
        exit(0);
    }
    else {
        printf("Unknown command. Type 'HELP' for available commands.\n");
    }
}

int main() {
    char input[MAX_INPUT_SIZE];
    char input_copy[MAX_INPUT_SIZE];
    char* args[MAX_ARGS];
    
    // Print welcome message and help
    printf("Welcome to NFS Client\n");
    printf("Type 'HELP' for available commands\n");
    
    // Main command loop
    while (1) {
        // Print prompt and get input
        printf("%s", PROMPT);
        fflush(stdout);
        
        // Read input
        if (!fgets(input, sizeof(input), stdin)) {
            printf("\nGoodbye!\n");
            break;
        }
        
        // Remove trailing newline
        input[strcspn(input, "\n")] = 0;
        
        // Skip empty lines
        if (strlen(input) == 0) continue;
        
        // Make a copy of input since parse_command modifies the string
        strncpy(input_copy, input, MAX_INPUT_SIZE - 1);
        input_copy[MAX_INPUT_SIZE - 1] = '\0';
        
        // Trim and parse the command
        char* trimmed_input = trim(input_copy);
        int argc = parse_command(trimmed_input, args);
        
        // Execute the command
        execute_command(argc, args);
    }
    
    return 0;
}