#include "utils.h"

// Initialize the global configuration variable
StorageServerConfig config;

// Trim leading and trailing whitespaces from a string
char* trim_whitespace(char* str) {
    if (str == NULL) return NULL;

    // Trim leading whitespaces
    while (isspace((unsigned char)*str)) str++;

    // Trim trailing whitespaces
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) {
        *end = '\0';
        end--;
    }

    return str;
}

// Parsing function
void parse_paths(char* paths_arg) {
    config.num_paths = 0;

    // Use a copy of the argument to avoid modifying original
    char paths_copy[BUFFER_SIZE];
    strncpy(paths_copy, paths_arg, sizeof(paths_copy) - 1);
    paths_copy[sizeof(paths_copy) - 1] = '\0';

    // First tokenization pass
    char* path_token = strtok(paths_copy, ",");
    
    while (path_token && config.num_paths < MAX_ACCESSIBLE_PATHS) {
        // Trim whitespaces
        char* trimmed_path = trim_whitespace(path_token);
        
        // Skip empty paths
        if (strlen(trimmed_path) == 0) {
            path_token = strtok(NULL, ",");
            continue;
        }

        // Validate path
        if (access(trimmed_path, F_OK) != 0) { // Check if path exists Relative Path too!
            fprintf(stderr, "Warning: Path '%s' does not exist\n", trimmed_path);
            return;
        }
        printf("Discovered %d paths:\n", config.num_paths);

        // Copy trimmed path
        strcpy(config.accessible_paths[config.num_paths], trimmed_path);
        config.num_paths++;

        // Get next path
        path_token = strtok(NULL, ",");
    }
}

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

// Function to get Storage Server IP (using one of the methods from previous answer)
char* get_storage_server_ip() {
    int new_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (new_sock < 0) {
        perror("socket");
        return NULL;
    }

    struct sockaddr_in serv;
    memset(&serv, 0, sizeof(serv));
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = inet_addr("8.8.8.8");  // Google's DNS server
    serv.sin_port = htons(53);  // DNS port

    if (connect(new_sock, (struct sockaddr *)&serv, sizeof(serv)) < 0) {
        perror("connect");
        close(new_sock);
        return NULL;
    }

    struct sockaddr_in name;
    socklen_t namelen = sizeof(name);
    char* ip_address = malloc(INET_ADDRSTRLEN);
    
    if (getsockname(new_sock, (struct sockaddr *)&name, &namelen) < 0) {
        perror("getsockname");
        close(new_sock);
        free(ip_address);
        return NULL;
    }

    inet_ntop(AF_INET, &name.sin_addr, ip_address, INET_ADDRSTRLEN);
    
    close(new_sock);
    return ip_address;
}

// Initialization function to be called at server startup
void initialize_file_locks() {
    for (int i = 0; i < MAX_CONCURRENT_FILES; i++) {
        file_locks[i].filename[0] = '\0';
        file_locks[i].ref_count = 0;
    }
}

// Function to get or create a lock for a specific file
FileLock* get_file_lock(const char* filename) {
    pthread_mutex_lock(&file_locks_mutex);
    
    // Try to find existing lock or empty slot
    for (int i = 0; i < MAX_CONCURRENT_FILES; i++) {
        if (file_locks[i].filename[0] == '\0' || 
            strcmp(file_locks[i].filename, filename) == 0) {
            
            // Initialize lock if not already done
            if (file_locks[i].filename[0] == '\0') {
                strncpy(file_locks[i].filename, filename, MAX_PATH_LENGTH - 1);
                pthread_mutex_init(&file_locks[i].mutex, NULL);
                file_locks[i].ref_count = 0;
            }
            
            file_locks[i].ref_count++;
            pthread_mutex_unlock(&file_locks_mutex);
            return &file_locks[i];
        }
    }
    
    pthread_mutex_unlock(&file_locks_mutex);
    return NULL; // No available locks
}

// Function to release file lock
void release_file_lock(FileLock* lock) {
    if (lock == NULL) return;

    pthread_mutex_lock(&file_locks_mutex);
    lock->ref_count--;

    // If no more references, reset the lock
    if (lock->ref_count <= 0) {
        pthread_mutex_destroy(&lock->mutex);
        lock->filename[0] = '\0';
        lock->ref_count = 0;
    }
    
    pthread_mutex_unlock(&file_locks_mutex);
}
