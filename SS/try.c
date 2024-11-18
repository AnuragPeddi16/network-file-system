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


#define MAX_PATH_LENGTH 1024
#define BUFFER_SIZE 4096
#define MAX_CLIENTS 100
#define MAX_ACCESSIBLE_PATHS 100

// Logging mechanism
void log_message(const char* message) {
    FILE* log_file = fopen("storage_server.log", "a");
    if (log_file) {
        time_t now;
        time(&now);
        fprintf(log_file, "[%s] %s\n", ctime(&now), message);
        fclose(log_file);
    }
}

// Storage Server Configuration Structure
typedef struct {
    char nm_ip[50];
    int nm_port;
    int ss_port;
    char accessible_paths[MAX_ACCESSIBLE_PATHS][MAX_PATH_LENGTH];
    int num_paths;
} StorageServerConfig;

// Global Configuration
StorageServerConfig config;

// Function to find a free port
int find_free_port() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // Bind to any available port [Otherwise Search with range of ports]

    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        socklen_t len = sizeof(addr);
        getsockname(sock, (struct sockaddr*)&addr, &len); //Assigned the Newly found Port to addr
        int port = ntohs(addr.sin_port); // Convert to host byte order
        close(sock);
        return port; // Return the port number
    }
    return -1;
    
    //Potential code to search for free port in a range
    
    // for (int port = min_port; port <= max_port; port++) {
    //     int sock = socket(AF_INET, SOCK_STREAM, 0);
    //     struct sockaddr_in addr;
    //     addr.sin_family = AF_INET;
    //     addr.sin_addr.s_addr = INADDR_ANY;
    //     addr.sin_port = htons(port);
    //     if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
    //         close(sock);
    //         return port;
    //     }
    //     close(sock);
    // }
    // return -1;
}

// Register with Naming Server
void register_with_naming_server() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in nm_addr;
    nm_addr.sin_family = AF_INET;
    nm_addr.sin_port = htons(config.nm_port);
    inet_pton(AF_INET, config.nm_ip, &nm_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&nm_addr, sizeof(nm_addr)) < 0) {
        perror("Registration connection failed");
        return;
    }

    // Prepare registration message
    char message[BUFFER_SIZE];
    char paths_str[BUFFER_SIZE] = "";
    for (int i = 0; i < config.num_paths; i++) {
        strcat(paths_str, config.accessible_paths[i]);
        if (i < config.num_paths - 1) strcat(paths_str, ",");
    }

    snprintf(message, sizeof(message), "REGISTER|%d|%s", config.ss_port, paths_str);
    send(sock, message, strlen(message), 0);

    char response[BUFFER_SIZE];
    recv(sock, response, sizeof(response), 0);
    printf("Naming Server Response: %s\n", response);

    close(sock);
}

// Asynchronous Write Handler
void handle_async_write(const char* path, const char* data, int is_sync) {
    FILE* file = fopen(path, is_sync ? "w" : "a");
    if (!file) {
        log_message("Async write failed: Unable to open file");
        return;
    }

    if (is_sync) {
        // Synchronous write
        fputs(data, file);
    } else {
        // Asynchronous write - append in chunks
        static int chunk_size = 1024;
        for (int i = 0; i < strlen(data); i += chunk_size) {
            char chunk[chunk_size + 1];
            strncpy(chunk, data + i, chunk_size);
            chunk[chunk_size] = '\0';
            fputs(chunk, file);
            usleep(100000); // Simulate async behavior
        }
    }
    fclose(file);
}

// Handle Client Requests
void* handle_client_request(void* client_socket_ptr) {
    int client_sock = *(int*)client_socket_ptr;
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_sock, buffer, sizeof(buffer), 0);
    buffer[bytes_read] = '\0';

    char* operation = strtok(buffer, "|");
    
    if (strcmp(operation, "READ") == 0) {
        char* path = strtok(NULL, "|");
        FILE* file = fopen(path, "r");
        if (file) {
            char file_content[BUFFER_SIZE];
            while (fgets(file_content, sizeof(file_content), file)) {
                send(client_sock, file_content, strlen(file_content), 0);
            }
            fclose(file);
        }
    }
    else if (strcmp(operation, "WRITE") == 0) {
        char* path = strtok(NULL, "|");
        char* data = strtok(NULL, "|");
        char* sync_flag = strtok(NULL, "|");
        
        int is_sync = (sync_flag && strcmp(sync_flag, "SYNC") == 0);
        handle_async_write(path, data, is_sync);
    }
    else if (strcmp(operation, "DELETE") == 0) {
        char* path = strtok(NULL, "|");
        remove(path);
    }
    else if (strcmp(operation, "INFO") == 0) {
        char* path = strtok(NULL, "|");
        struct stat file_stat;
        if (stat(path, &file_stat) == 0) {
            char info[BUFFER_SIZE];
            snprintf(info, sizeof(info), "Size: %ld bytes, Permissions: %o", 
                     file_stat.st_size, file_stat.st_mode & 0777);
            send(client_sock, info, strlen(info), 0);
        }
    }
    else if (strcmp(operation, "STREAM") == 0) {
        // Audio streaming implementation
        char* path = strtok(NULL, "|");
        FILE* audio_file = fopen(path, "rb");
        if (audio_file) {
            char audio_buffer[BUFFER_SIZE];
            size_t bytes_read;
            while ((bytes_read = fread(audio_buffer, 1, sizeof(audio_buffer), audio_file)) > 0) {
                send(client_sock, audio_buffer, bytes_read, 0);
            }
            fclose(audio_file);
        }
    }

    close(client_sock);
    free(client_socket_ptr);
    return NULL;
}

int main(int argc, char *argv[]) {
    // Validate command line arguments Format ./server <Naming_Server_IP> <Accessible_Paths[Separated by commas]>
    if (argc < 3) {
        printf("Usage: %s <Naming_Server_IP> <Accessible_Paths>\n", argv[0]);
        return 1;
    }

    // Initialize Configuration
    strcpy(config.nm_ip, argv[1]);
    config.nm_port = 5050;  // Default NM port
    config.ss_port = find_free_port();

    // Parse Accessible Paths, Tiokenise with , and store in config
    char* path_token = strtok(argv[2], ",");
    config.num_paths = 0;
    while (path_token && config.num_paths < MAX_ACCESSIBLE_PATHS) {
        strcpy(config.accessible_paths[config.num_paths], path_token);
        config.num_paths++;
        path_token = strtok(NULL, ",");
    }

    // Register with Naming Server
    register_with_naming_server();

    // Create a socket to listen for client connections
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.ss_port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        return 1;
    }

    listen(server_sock, MAX_CLIENTS);
    printf("Storage Server running on port %d\n", config.ss_port);

    // Accept client connections and handle requests
    while (1) {
        int* client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, NULL, NULL);
        if (*client_sock < 0) {
            perror("Failed to accept client connection");
            free(client_sock);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client_request, client_sock) != 0) {
            perror("Failed to create thread");
            close(*client_sock);
            free(client_sock);
        }
        pthread_detach(client_thread); // Detach the thread to avoid memory leaks
    }

    close(server_sock);
    return 0;
}