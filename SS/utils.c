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
