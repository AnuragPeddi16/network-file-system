#include "utils.h"
#include "operations.h"

extern StorageServerConfig config;

int sock;

// Register with Naming Server
void register_with_naming_server() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
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

    //Format: REGISTER|<SS_PORT>:<PATH1>,<PATH2>,<PATH3>...
    snprintf(message, sizeof(message), "REGISTER|%d:%s", config.ss_nm_port, paths_str);
    send(sock, message, strlen(message), 0);

    char response[BUFFER_SIZE];
    recv(sock, response, sizeof(response), 0); //Waiting for Acknowledgement
    printf("Naming Server Response: %s\n", response);
}

int main(int argc, char *argv[]) {
    // Validate command line arguments Format ./server <Naming_Server_IP> <Accessible_Paths[Separated by commas]>
    if (argc < 3) {
        printf("Usage: %s <Naming_Server_IP> <Accessible_Paths>\n", argv[0]);
        return 1;
    }

    // Initialize Configuration
    strcpy(config.nm_ip, argv[1]);
    config.nm_port = NM_PORT;  // Default NM port
    config.ss_nm_port = find_free_port();
    config.ss_client_port = find_free_port();

    //Error Check: Free Port
    if(config.ss_nm_port == -1 || config.ss_client_port == -1){
        printf("Error: Unable to find free port\n");
        return 1;
    }

    // Parse Accessible Paths, Tokenise with , and store in config
    parse_paths(argv[2]);

    // Register with Naming Server
    register_with_naming_server();

    // Create a socket to listen for client connections
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(config.ss_client_port);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Failed to bind socket");
        return 1;
    }

    listen(server_sock, MAX_CLIENTS);
    printf("Storage Server running on port %d\n", config.ss_client_port);

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