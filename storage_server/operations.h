// operations.h
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
#include "../macros.h"

/* // Status codes
#define OK 0
#define ACK 1
#define NOT_FOUND 2
#define FAILED 3
#define ASYNCHRONOUS_COMPLETE 4
#define SYNC_THRESHOLD 4096  // 4KB */
#define SEGMENT_SIZE 1024    // 1KB chunks for async writing

// Function prototypes
void* handle_client_request(void* client_socket_ptr);
int handle_client_read_request(const char* path, int client_socket);
int handle_client_write_request(const char* path, const char* data);
int handle_client_create_request(const char* type,const char* path);
int handle_client_delete_request(const char* type,const char* path);
int handle_client_info_request(const char* path, char* info_buffer);
int handle_client_stream_request(const char* path, int client_socket);
int handle_client_list_request(int client_socket);
int zip_path_and_send(const char* source_path);
int unzip_received_data(const char* input);

#endif // OPERATIONS_H