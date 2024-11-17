#ifndef OPERATIONS_H
#define OPERATIONS_H

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
#include <time.h>
#include "utils.h"

#define MAX_PATH_LENGTH 1024
#define BUFFER_SIZE 4096
#define MAX_ACCESSIBLE_PATHS 100

// Status codes
#define OK "0"
#define ACK "1"
#define NOT_FOUND "2"
#define FAILED "3"
#define SYNC_THRESHOLD 4096  // 4KB
#define CHUNK_SIZE 1024      // 1KB chunks for async writing

//nm_sock
extern int sock;

// Function prototypes
void* handle_client_request(void* client_socket_ptr);
int handle_client_read_request(const char* path, int client_socket);
int handle_client_write_request(const char* path, const char* data);
int handle_client_create_request(const char* path);
int handle_client_delete_request(const char* path);
int handle_client_info_request(const char* path, char* info_buffer);
int handle_client_stream_request(const char* path, int client_socket);
int handle_client_list_request(int client_socket);
int copy_file(const char* source_path, const char* dest_path);
int copy_directory(const char* source_path, const char* dest_path);
int handle_client_copy_request(char* type, char* source_path, const char* dest_path);

#endif // OPERATIONS_H