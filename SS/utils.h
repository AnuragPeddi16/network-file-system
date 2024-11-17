#ifndef UTILS_H
#define UTILS_H

//Header Files
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <ctype.h>
#include "utils.h"

// Define constants
#define BUFFER_SIZE 4096
#define MAX_ACCESSIBLE_PATHS 100
#define MAX_PATH_LENGTH 1024
#define MAX_CLIENTS 100

// Storage Server Configuration Structure
typedef struct {
    char nm_ip[50];
    int nm_port;
    int ss_nm_port;     // Port for Naming Server connections
    int ss_client_port; // Port for client connections
    char accessible_paths[MAX_ACCESSIBLE_PATHS][BUFFER_SIZE];
    int num_paths;
} StorageServerConfig;

// Global configuration variable
extern StorageServerConfig config;

// Function declarations
char* trim_whitespace(char* str);
void parse_paths(char* paths_arg);
void log_message(const char* message);
int find_free_port();

#endif // UTILS_H