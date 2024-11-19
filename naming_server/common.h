#ifndef _COMMON_H_
#define _COMMON_H_

#define _POSIX_C_SOURCE 200112L

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <poll.h>

#include "../macros.h"
#include "tries.h"
#include "log.h"

#define MAX_SERVERS 50
#define MAX_CLIENTS 50
#define MAX_PATHS_PER_SERVER 100

#define BUFFER_SIZE 4096
#define MAX_FILE_SIZE 100000

#define SS_PORT 8050
#define CLIENT_PORT 8051


// Structure to store metadata about storage servers
typedef struct StorageServer {
    int fd; // active connection
    bool active;
    char ip[INET_ADDRSTRLEN];
    int nm_port;
    int client_port;
    TrieNode* paths_root;
    struct StorageServer* backups[2];
    int num_backups;
} StorageServer;

extern StorageServer storage_servers[MAX_SERVERS];
extern int server_count;
extern pthread_mutex_t server_mutex;

typedef struct {

    int status;
    char server_ip[INET_ADDRSTRLEN];
    int server_port;

} ClientResponse;

void print_error(char* message);
char* str_before_last_slash(char* str);
char** tokenize(char* string, char* delimiters);
void completeFree(char** arr);
int is_socket_connected(int sock_fd);

#endif