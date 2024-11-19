#include "utils.h"
#include "operations.h"

StorageServerConfig config; // Global configuration variable, Declare in Utils.h
int sock;

// Global array to track file locks
FileLock file_locks[MAX_CONCURRENT_FILES];
pthread_mutex_t file_locks_mutex;

// Register with Naming Server
void register_with_naming_server() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in nm_addr;
    nm_addr.sin_family = AF_INET;
    pthread_mutex_lock(&config.config_mutex);
    nm_addr.sin_port = htons(config.nm_port);
    inet_pton(AF_INET, config.nm_ip, &nm_addr.sin_addr);
    pthread_mutex_unlock(&config.config_mutex);

    if (connect(sock, (struct sockaddr*)&nm_addr, sizeof(nm_addr)) < 0) {
        log_message("Registration connection failed");
        perror("Connection failed");
        return;
    }

    // Prepare registration message
    char message[BUFFER_SIZE+100];
    char paths_str[BUFFER_SIZE] = "";
    pthread_mutex_lock(&config.config_mutex);
    for (int i = 0; i < config.num_paths; i++) {
        strcat(paths_str, config.accessible_paths[i]);
        if (i < config.num_paths - 1) strcat(paths_str, ",");
    }
    pthread_mutex_unlock(&config.config_mutex);

    //Format: REGISTER|<SS_PORT>:<PATH1>,<PATH2>,<PATH3>...
    pthread_mutex_lock(&config.config_mutex);
    snprintf(message, sizeof(message), "REGISTER|%d:%s", config.ss_client_port, paths_str);
    pthread_mutex_unlock(&config.config_mutex);
    send(sock, message, strlen(message), 0);
    //I can add IP here, if required

    char response[BUFFER_SIZE];
    recv(sock, response, sizeof(response), 0); //Waiting for Acknowledgement
    if(response[0] != '1'){
        log_message("FAILED:Registration");
        return;
    }
    else{
        log_message("SUCCESS:Registration");
    }
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

    pthread_mutex_init(&config.config_mutex, NULL);

    pthread_mutex_lock(&config.config_mutex);
    //Error Check: Free Port
    if(config.ss_nm_port == -1 || config.ss_client_port == -1){
        log_message("Error: Unable to find free port");
        return 1;
    }
    pthread_mutex_unlock(&config.config_mutex);

    // Parse Accessible Paths, Tokenise with , and store in config
    parse_paths(argv[2]);

    // Register with Naming Server
    register_with_naming_server();

    //Initialize locks
    initialize_file_locks();

    // Create a socket to listen for client connections
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    pthread_mutex_lock(&config.config_mutex);
    server_addr.sin_port = htons(config.ss_client_port);
    pthread_mutex_unlock(&config.config_mutex);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        log_message("Error: Failed to bind Client socket");
        return 1;
    }

    listen(server_sock, MAX_CLIENTS);
    char message[BUFFER_SIZE];

    pthread_mutex_lock(&config.config_mutex);
    sprintf(message, "SUCCESS: Storage Server running on port Client_port %d", config.ss_client_port);
    pthread_mutex_unlock(&config.config_mutex);
    
    log_message(message);

    // Accept client connections and handle requests
    while (1) {
        int* client_sock = malloc(sizeof(int));
        *client_sock = accept(server_sock, NULL, NULL);
        if (*client_sock < 0) {
            log_message("Failed to accept client connection");
            free(client_sock);
            continue;
        }

        pthread_t client_thread;
        if (pthread_create(&client_thread, NULL, handle_client_request, client_sock) != 0) {
            log_message("Failed to create thread");
            close(*client_sock);
            free(client_sock);
        }
        pthread_detach(client_thread); // Detach the thread to avoid memory leaks
    }

    close(server_sock);
    return 0;
}