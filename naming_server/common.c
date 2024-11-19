#include "common.h"

void print_error(char* message) {

    char* str = (char*) malloc(sizeof(char)*(strlen(message)+50));
    sprintf(str, RED("%s"), message);
    perror(str);
    log_message(message);

    free(str);

}

// includes the slash
char* str_before_last_slash(char* str) {

    char* substring = NULL;
    char* last = strrchr(str, '/');

    if (last != NULL) {

        int length = last - str + 1;
        substring = malloc(length+1);
        strncpy(substring, str, length);
        substring[length] = '\0';

    }

    return substring;

}

char** tokenize(char* string, char* delimiters) {

    char** token_arr = (char**) malloc(sizeof(char*)*MAX_PATHS_PER_SERVER);
    memset(token_arr, 0, sizeof(char*)*MAX_PATHS_PER_SERVER);

    char* token;
    token = strtok(string, delimiters);

    int i = 0;
    while (token) {

        token_arr[i] = (char*) malloc(sizeof(char)*(strlen(token)+1));
        strcpy(token_arr[i], token);

        token = strtok(NULL, delimiters);
        i++;
 
    }

    return token_arr;

}

void completeFree(char** arr) {

    for (int i = 0; arr[i]; i++) {

        free(arr[i]);

    }

    free(arr);

}

int is_socket_connected(int sock_fd) {
    char buffer;
    ssize_t result = recv(sock_fd, &buffer, 1, MSG_PEEK | MSG_DONTWAIT);
    
    if (result > 0) {
        // Data is available; the socket is still connected.
        return 1;
    } else if (result == 0) {
        // Connection has been closed by the peer.
        return 0;
    } else {
        // Check for specific errors.
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            // No data available, but the socket is still connected.
            return 1;
        } else {
            // An error occurred, socket is likely disconnected.
            return 0;
        }
    }
}