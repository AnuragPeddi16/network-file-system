#include "operations.h"

// Function to handle client requests
void* handle_client_request(void* client_socket_ptr) {
    int client_sock = *(int*)client_socket_ptr;
    char buffer[BUFFER_SIZE];
    int bytes_read = recv(client_sock, buffer, sizeof(buffer), 0);
    
    if (bytes_read <= 0) {
        close(client_sock);
        free(client_socket_ptr);
        return NULL;
    }
    buffer[bytes_read] = '\0';

    // Tokenize the request
    char* operation = strtok(buffer, " ");
    char* path = strtok(NULL, " ");  // For CREATE/DELETE/COPY It will be FILE/FOLDER

    // Handle different operations
    if (operation == NULL) {
        send(client_sock, FAILED, strlen(FAILED), 0);
    } 
    else if (strcmp(operation, "READ") == 0) {
        if (handle_client_read_request(path, client_sock) == 0) {
            send(client_sock, ACK, strlen(ACK), 0);
        } 
        else {
            send(client_sock, NOT_FOUND, strlen(NOT_FOUND), 0);
        }
    } 
    else if (strcmp(operation, "WRITE") == 0) {
        char* data = strtok(NULL, " ");  // For WRITE operation, this will be the data to write
        if (data) {
            // Get data size
            size_t data_size = strlen(data);
            
            //Check if file exists
            FILE* file = fopen(path, "w");
            if (!file) {
                send(client_sock, NOT_FOUND, strlen(NOT_FOUND), 0);
                return -1;
            }
            fclose(file);

            //Write data to file
            if (data_size >= SYNC_THRESHOLD) {              //Asynchronous Write
                send(client_sock, ACK, strlen(ACK), 0);
            }
            if (handle_client_write_request(path, data) != 0) {
                send(client_sock, FAILED, strlen(FAILED), 0); 
            }
            if (data_size < SYNC_THRESHOLD) {
                send(client_sock, ACK, strlen(ACK), 0);     //Synchronous Write
            }
        } 
        else {
            send(client_sock, FAILED, strlen(FAILED), 0);
        }
    } 
    else if (strcmp(operation, "CREATE") == 0) {
        if (handle_client_create_request(path) == 0) {
            send(client_sock, ACK, strlen(ACK), 0);
            send(sock, ACK, strlen(ACK), 0);
        } else {
            send(client_sock, FAILED, strlen(FAILED), 0);
            send(sock, FAILED, strlen(FAILED), 0);
        }
    }
    else if (strcmp(operation, "DELETE") == 0) {
        if (handle_client_delete_request(path) == 0) {
            send(client_sock, ACK, strlen(ACK), 0);
            send(sock, ACK, strlen(ACK), 0);
        } else {
            send(client_sock, FAILED, strlen(FAILED), 0);
            send(sock, FAILED, strlen(FAILED), 0);
        }
    } 
    else if (strcmp(operation, "INFO") == 0) {
        char info_buffer[BUFFER_SIZE];
        if (handle_client_info_request(path, info_buffer) == 0) {
            send(client_sock, info_buffer, strlen(info_buffer), 0);
            send(client_sock, ACK, strlen(ACK), 0);
        } else {
            send(client_sock, NOT_FOUND, strlen(NOT_FOUND), 0);
        }
    } 
    else if (strcmp(operation, "STREAM") == 0) {
        if (handle_client_stream_request(path, client_sock) == 0) {
            send(client_sock, ACK, strlen(ACK), 0);
        } else {
            send(client_sock, NOT_FOUND, strlen(NOT_FOUND), 0);
        }
    } 
    else if (strcmp(operation, "LIST") == 0) {
        handle_client_list_request(client_sock);
        send(client_sock, ACK, strlen(ACK), 0);
    } 
    else if (strcmp(operation, "COPY") == 0) {
        char* source_path = strtok(NULL, " ");
        char* dest_path = strtok(NULL, " ");
        if (source_path && dest_path) {
            if (handle_client_copy_request(path, source_path, dest_path) == 0) {
                send(client_sock, ACK, strlen(ACK), 0);
            } 
            else {
                send(client_sock, FAILED, strlen(FAILED), 0);
            }
        } 
        else {
            send(client_sock, FAILED, strlen(FAILED), 0);
        }
    } 
    else {
        send(client_sock, FAILED, strlen(FAILED), 0);
    }

    close(client_sock);
    free(client_socket_ptr);
    return NULL;
}

// Handle READ request
int handle_client_read_request(const char* path, int client_socket) {
    char buffer[BUFFER_SIZE];
    FILE* file = fopen(path, "r");
    if (!file) {
        return -1; // File not found
    }

    while (fgets(buffer, sizeof(buffer), file)) {
        send(client_socket, buffer, strlen(buffer), 0);
    }
    fclose(file);
    return 0;
}

// Handle WRITE request with sync/async decision based on data size
int handle_client_write_request(const char* path, const char* data) {
    FILE* file = fopen(path, "w");
    if (!file) {
        log_message("ERROR: Unable to open file for synchronous writing");
        return -1;
    }
    fputs(data, file);
    fclose(file);
    return 0;
}

// Handle CREATE request
int handle_client_create_request(const char* path) {
    // If you want to tokenize `path` further
    char path_copy[MAX_PATH_LENGTH];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    // Tokenize to type
    char* type = strtok(path_copy, " ");
    if (strcmp(type, "FILE") == 0) {
        char* actual_path = strtok(NULL, " ");
        // Create an empty file (original implementation)
        FILE* file = fopen(actual_path, "w");
        if (!file) {
            log_message("ERROR: Unable to create file");
            return -1;
        }
        fclose(file);
        return 0;
    }
    else if (strcmp(type, "FOLDER") == 0) {
        char* actual_path = strtok(NULL, " ");
        // Create directory with 0755 permissions
        if (mkdir(actual_path, 0755) != 0) {
            log_message("ERROR: Unable to create folder");
            return -1;
        }
        return 0;
    }
    else {
        log_message("ERROR: Unknown creation type");
        return -1;
    }   
}

// Handle DELETE request
int handle_client_delete_request(const char* path) {
    // If you want to tokenize `path` further
    char path_copy[MAX_PATH_LENGTH];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    // Tokenize to type
    char* type = strtok(path_copy, " ");
    char* actual_path = strtok(NULL, " ");

    //Check
    if (type == NULL || actual_path == NULL) {
        log_message("ERROR: Invalid delete request");
        return -1;
    }

    if (strcmp(type, "FILE") == 0) {
        
        // Delete a file
        if (remove(actual_path)) {
            return 0;
        }
        log_message("ERROR: Unable to Delete file");
        return -1;
    }
    else if (strcmp(type, "FOLDER") == 0) {
        // Delete a directory
        if (rmdir(actual_path) == 0) {
            return 0;
        }
        log_message("ERROR: Unable to create folder");
        return -1;
        
    }
    else {
        log_message("ERROR: Unknown Deletion type");
        return -1;
    }
}

// Handle INFO request
int handle_client_info_request(const char* path, char* info_buffer) {
    struct stat file_stat;
    if (stat(path, &file_stat) != 0) {
        return -1; // File not found
    }
    snprintf(info_buffer, BUFFER_SIZE, 
             "Size: %ld bytes, Permissions: %o, Modified: %s", 
             file_stat.st_size, 
             file_stat.st_mode & 0777, 
             ctime(&file_stat.st_mtime));
    return 0;
}

// Handle STREAM request
int handle_client_stream_request(const char* path, int client_socket) {
    FILE* audio_file = fopen(path, "rb");
    if (!audio_file) {
        send(client_socket, "ERROR: Unable to open audio file", 34, 0);
        return -1;
    }

    char audio_buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(audio_buffer, 1, sizeof(audio_buffer), audio_file)) > 0) {
        send(client_socket, audio_buffer, bytes_read, 0);
    }
    fclose(audio_file);
    return 0;
}

// Handle LIST request
int handle_client_list_request(int client_socket) {
    char response[BUFFER_SIZE] = "Accessible Paths:\n";
    for (int i = 0; i < config.num_paths; i++) {
        strcat(response, config.accessible_paths[i]);
        strcat(response, "\n");
    }
    send(client_socket, response, strlen(response), 0);
    return 0;
}

// Handle COPY request
int copy_file(const char* source_path, const char* dest_path) {
    FILE* source = fopen(source_path, "rb");
    if (!source) {
        log_message("ERROR: Cannot open source file");
        return -1;
    }

    FILE* dest = fopen(dest_path, "wb");
    if (!dest) {
        fclose(source);
        log_message("ERROR: Cannot create destination file");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), source)) > 0) {
        fwrite(buffer, 1, bytes_read, dest);
    }

    fclose(source);
    fclose(dest);
    return 0;
}

int copy_directory(const char* source_path, const char* dest_path) {
    DIR* dir = opendir(source_path);
    if (!dir) {
        log_message("ERROR: Cannot open source directory");
        return -1;
    }

    // Create destination directory
    mkdir(dest_path, 0755);

    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;

        char source_file[MAX_PATH_LENGTH];
        char dest_file[MAX_PATH_LENGTH];
        
        snprintf(source_file, sizeof(source_file), "%s/%s", source_path, entry->d_name);
        snprintf(dest_file, sizeof(dest_file), "%s/%s", dest_path, entry->d_name);

        struct stat path_stat;
        stat(source_file, &path_stat);

        if (S_ISDIR(path_stat.st_mode)) {
            // Recursive directory copy
            copy_directory(source_file, dest_file);
        } else {
            // File copy
            copy_file(source_file, dest_file);
        }
    }

    closedir(dir);
    return 0;
}

int handle_client_copy_request(char* type, char* source_path, const char* dest_path) {
    if (strcmp(type, "FILE") == 0) {
        return copy_file(source_path, dest_path);
    } else if (strcmp(type, "FOLDER") == 0) {
        return copy_directory(source_path, dest_path);
    } else {
        log_message("ERROR: Unknown copy type");
        return -1;
    }
}