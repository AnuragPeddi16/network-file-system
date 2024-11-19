#include "common.h"
#include "handle_client.h"
#include "lru.h"

StorageServer storage_servers[MAX_SERVERS];
int server_count = 0;
pthread_mutex_t server_mutex;

// Function to get the local IPv4 address
void get_local_ipv4_address(char *ip_buffer, size_t buffer_size) {
    struct ifaddrs *ifaddr, *ifa;
    int family;

    if (getifaddrs(&ifaddr) == -1) {
        print_error("getifaddrs");
        exit(EXIT_FAILURE);
    }

    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;
        if (family == AF_INET) { // IPv4
            getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), ip_buffer, buffer_size, NULL, 0, NI_NUMERICHOST);
            if (strcmp(ip_buffer, "127.0.0.1") != 0) { // Ignore loopback address
                break;
            }
        }
    }

    freeifaddrs(ifaddr);
}

void tokenise_and_store(TrieNode* root, char* paths) {

    char** path_tokens = tokenize(paths, ",");

    int i = 0;
    while (path_tokens[i] != NULL) {
        
        insert_path_trie(root, path_tokens[i]);
        i++;

    }

    completeFree(path_tokens);

}

// Function to add a storage server
bool add_storage_server(int fd, const char *ip, int nm_port, int client_port, char *paths) {

    pthread_mutex_lock(&server_mutex);

    if (server_count >= MAX_SERVERS) {

        print_error("Max server limit reached.\n");
        pthread_mutex_unlock(&server_mutex);
        return false;

    }

    storage_servers[server_count].fd = fd;
    storage_servers[server_count].active = true;
    strncpy(storage_servers[server_count].ip, ip, INET_ADDRSTRLEN);
    storage_servers[server_count].nm_port = nm_port;
    storage_servers[server_count].client_port = client_port;

    storage_servers[server_count].paths_root = create_node("root", false);
    tokenise_and_store(storage_servers[server_count].paths_root, paths);

    server_count++;

    char message[BUFFER_SIZE];
    sprintf(message, "Storage server added: IP %s, Port %d\n\n", ip, nm_port);
    log_message(message);

    pthread_mutex_unlock(&server_mutex);
    return true;

}

// Thread function to accept incoming client connections
void *accept_ss_connections(void *args) {

    int nm_fd;
    struct sockaddr_in server_addr;

    // Create a socket
    if ((nm_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {

        print_error("Socket creation failed");
        exit(EXIT_FAILURE);

    }

    int opt = 1;
    if (setsockopt(nm_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        print_error("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SS_PORT);
    if (bind(nm_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {

        print_error("Bind failed");
        close(nm_fd);
        exit(EXIT_FAILURE);

    }

    // Listen for incoming connections
    if (listen(nm_fd, MAX_SERVERS) < 0) {

        print_error("Listen failed");
        close(nm_fd);
        exit(EXIT_FAILURE);

    }

    char ip[INET_ADDRSTRLEN];
    get_local_ipv4_address(ip, INET_ADDRSTRLEN);

    char message[BUFFER_SIZE];
    sprintf(message, "Naming server listening for storage servers on " "port %d" " at " "ip %s\n\n", SS_PORT, ip);
    log_message(message);

    printf(GREEN("Thread started:") " Waiting for incoming storage server connections...\n\n");

    while (1) {

        int ss_fd;
        struct sockaddr_in ss_addr;
        socklen_t ss_addr_len = sizeof(ss_addr);

        ss_fd = accept(nm_fd, (struct sockaddr *)&ss_addr, &ss_addr_len);
        if (ss_fd < 0) {

            print_error("Error accepting connection");
            continue;

        }

        log_message("Received server connection...\n");

        // Receive data from the storage server
        char buffer[PATH_SIZE*MAX_PATHS_PER_SERVER] = {0};
        int bytes_received = recv(ss_fd, buffer, PATH_SIZE*MAX_PATHS_PER_SERVER - 1, 0);
        if (bytes_received < 0) {
            print_error("Error receiving data");
            close(ss_fd);
            continue;
        }

        buffer[bytes_received] = '\0'; // Null-terminate the received string

        // Parse the received data (format: "REGISTER|<PORT_NUMBER>:<PATH1>,<PATH2>,<PATH3>...")
        int client_port;
        char paths[PATH_SIZE*MAX_PATHS_PER_SERVER] = {0};
        sscanf(buffer+9, "%d:%[^\n]", &client_port, paths);

        add_storage_server(ss_fd, inet_ntoa(ss_addr.sin_addr), ntohs(ss_addr.sin_port), client_port, paths);

        int ack = htonl(ACK);
        if (send(ss_fd, &ack, sizeof(ack), 0) < 0) {

            print_error("sending ack to ss failed");

        }
        
    }

}

// Thread function to accept incoming ss connections
void *accept_client_connections(void *args) {

    int nm_fd;
    struct sockaddr_in server_addr;

    // Create a socket
    if ((nm_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {

        print_error("Socket creation failed");
        exit(EXIT_FAILURE);

    }

    int opt = 1;
    if (setsockopt(nm_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        print_error("setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind the socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(CLIENT_PORT);
    if (bind(nm_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {

        print_error("Bind failed");
        close(nm_fd);
        exit(EXIT_FAILURE);

    }

    // Listen for incoming connections
    if (listen(nm_fd, MAX_CLIENTS) < 0) {

        print_error("Listen failed");
        close(nm_fd);
        exit(EXIT_FAILURE);

    }

    char ip[INET_ADDRSTRLEN];
    get_local_ipv4_address(ip, INET_ADDRSTRLEN);

    char message[BUFFER_SIZE];
    sprintf(message, "Naming server listening for clients on " "port %d" " at " "ip %s\n\n", CLIENT_PORT, ip);
    log_message(message);

    printf(GREEN("Thread started:") " Waiting for incoming client connections...\n\n");

    while (1) {

        int client_fd;
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        client_fd = accept(nm_fd, (struct sockaddr *)&client_addr, &client_addr_len);
        if (client_fd < 0) {

            print_error("Error accepting connection");
            continue;

        }


        char message[BUFFER_SIZE];
        sprintf(message, "Accepted client connection %d\n", client_fd);
        log_message(message);

        pthread_t client_thread;
        pthread_create(&client_thread, NULL, client_handler, (void*)&client_fd);
        pthread_detach(client_thread);
        
    }

}

// Function to initialize the server and listen for connections
void start_naming_server() {
    
    pthread_t server_connect_thread, client_connect_thread;
    pthread_create(&server_connect_thread, NULL, accept_ss_connections, NULL);
    pthread_create(&client_connect_thread, NULL, accept_client_connections, NULL);

    pthread_join(server_connect_thread, NULL);
    pthread_join(client_connect_thread, NULL);
    
}

int main() {

    signal(SIGPIPE, SIG_IGN);

    pthread_mutex_init(&server_mutex, NULL);

    init_log();
    cache_init();

    start_naming_server();

    cache_cleanup();

    pthread_mutex_destroy(&server_mutex);

    return 0;

}
