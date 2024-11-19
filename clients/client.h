#ifndef NFS_CLIENT_H
#define NFS_CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define BUFFER_SIZE 1024
#define MAX_PATH_LENGTH 256
#define NAMING_SERVER_PORT 8051  // Example port
#define MAX_RESPONSE_SIZE 4096

// Operation codes
#define OP_READ 1
#define OP_WRITE 2
#define OP_CREATE 3
#define OP_DELETE 4
#define OP_LIST 5
#define OP_INFO 6
#define OP_STREAM 7
#define OP_COPY 8

// Structure to hold server information
typedef struct ServerInfo {
    int status;
    char ip[INET_ADDRSTRLEN];
    int port;
} ServerInfo;

// Function declarations
int connect_to_server(const char* ip, int port);
ServerInfo get_storage_server_info(int naming_server_fd, const char* path, int operation);
int handle_read_operation(const char* path);
int handle_write_operation(const char* path, const char* content);
int handle_create_operation(const char* path, const char* name);
int handle_delete_operation(const char* path);
int handle_list_operation(const char* path);
int handle_stream_operation(const char* path);
int handle_copy_operation(const char* source, const char* dest);

#endif
