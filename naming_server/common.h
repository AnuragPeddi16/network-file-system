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

#include "tries.h"

#define MAX_SERVERS 50
#define MAX_CLIENTS 50
#define MAX_PATHS_PER_SERVER 500

#define BUFFER_SIZE 4096

#define SS_PORT 8050
#define CLIENT_PORT 8051


// Structure to store metadata about storage servers
typedef struct {
    int fd; // active connection
    char ip[INET_ADDRSTRLEN];
    int nm_port;
    int client_port;
    //char accessible_paths[PATH_SIZE*MAX_PATHS_PER_SERVER]; // comma separated
    TrieNode* paths_root;
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

#endif