// utils.h
#ifndef UTILS_H
#define UTILS_H

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
#include <time.h>

// Define constants
#define BUFFER_SIZE 4096
#define MAX_ACCESSIBLE_PATHS 100
#define MAX_PATH_LENGTH 1024
#define MAX_CLIENTS 100
#define NM_PORT 8050
#define MAX_CONCURRENT_FILES 100

// Storage Server Configuration Structure
typedef struct {
    char nm_ip[50];
    int nm_port;
    int ss_nm_port;     // Port for Naming Server connections
    int ss_client_port; // Port for client connections
    char accessible_paths[MAX_ACCESSIBLE_PATHS][BUFFER_SIZE];
    int num_paths;
} StorageServerConfig;

// Structure to manage file locks
typedef struct {
    char filename[MAX_PATH_LENGTH];
    pthread_mutex_t mutex;
    int ref_count;
} FileLock;

// Global configuration variable
extern StorageServerConfig config;
extern int sock;

// Global array to track file locks
extern FileLock file_locks[];
extern pthread_mutex_t file_locks_mutex;

// Function declarations
char* trim_whitespace(char* str);
void parse_paths(char* paths_arg);
void log_message(const char* message);
int find_free_port();
char* get_storage_server_ip();
void initialize_file_locks();
FileLock* get_file_lock(const char* filename);
void release_file_lock(FileLock* lock);

#endif // UTILS_H