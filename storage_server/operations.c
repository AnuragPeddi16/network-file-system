#include "operations.h"

// Enhanced write request handler with file-level locking
int handle_client_write_request(const char* upath, const char* data) {
    char* path=malloc(sizeof(upath)+MAX_PATH_LENGTH);
    strcpy(path,config.base_path);
    strcat(path,upath);

    int sync=0;
    const char *substring = "SYNC";
    if (strstr(path, substring) != NULL) {
        sync=1;
        strtok(path, "-");
        log_message("SYNC Write Request");
    }

    // Get file-specific lock
    // FileLock* file_lock = get_file_lock(upath);
    // if (!file_lock) {
    //     log_message("ERROR: Unable to acquire file lock");
    //     return -1;
    // }

    // Acquire lock for this specific file
    // pthread_mutex_lock(&file_lock->mutex);

    size_t data_size = strlen(data);
    
    // Log the write attempt
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Write Request: Path=%s, Size=%zu bytes", path, data_size);
    log_message(log_msg);

    FILE* file = fopen(path, "w");
    if (!file) {
        log_message("ERROR: Unable to open file for synchronous writing");
        // pthread_mutex_unlock(&file_lock->mutex);
        // release_file_lock(file_lock);
        return -1;
    }

    int write_result = 0;

    // Choose writing strategy based on data size
    if (data_size > ASYNC_LIMIT && sync==0) {
        // Segmented writing for large data
        size_t written = 0;
        while (written < data_size) {
            size_t remaining = data_size - written;
            size_t write_size = (remaining < SEGMENT_SIZE) ? remaining : SEGMENT_SIZE;
            
            // Write segment
            size_t segment_written = fwrite(data + written, 1, write_size, file);
            
            if (segment_written != write_size) {
                // Error during writing
                log_message("ERROR: Partial write during segmented writing");
                write_result = -1;
                break;
            }
            fflush(file);
            written += segment_written;
            usleep(10000);  // 10 milliseconds
        }
    } 
    else {
        // Single-go writing for small data
        size_t written = fwrite(data, 1, data_size, file);
        
        if (written != data_size) {
            log_message("ERROR: Incomplete write in single-go");
            write_result = -1;
        }
    }

    // Ensure all data is written to disk
    if (fflush(file) != 0) {
        log_message("WARNING: Data may not be fully flushed to disk");
    }
    fclose(file);

    // Release file-specific lock
    // pthread_mutex_unlock(&file_lock->mutex);
    // release_file_lock(file_lock);

    if (write_result == 0) {
        // Log successful write
        snprintf(log_msg, sizeof(log_msg), "WRITE SUCCESS: Path=%s, Size=%zu bytes", path, data_size);
        log_message(log_msg);
    }

    return write_result;
}

// Enhanced read request handler with file-level locking
int handle_client_read_request(const char* upath, int client_socket) {
    char* path=malloc(sizeof(upath)+MAX_PATH_LENGTH);
    strcpy(path,config.base_path);
    strcat(path,upath);

    char buffer[BUFFER_SIZE];
    FILE* file = fopen(path, "r");
    
    // Log the read attempt
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Read Request: Path=%s", path);
    log_message(log_msg);

    if (!file) {
        log_message("ERROR: Unable to open file for Reading");
        return -1; // File not found
    }
    while (fgets(buffer, sizeof(buffer), file)) {
        send(client_socket, buffer, strlen(buffer), 0);
    }
    send(client_socket,"STOP",4, 0);

    fclose(file);
    log_message("READ SUCCESS");
    return 0;
}

int handle_client_create_request(const char* type,const char* path) { 
    // Log the Create attempt
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Create Request:%s Path=%s",type,path);
    log_message(log_msg);

    //Tokenise path
    char* actual_path=malloc(sizeof(path)+MAX_PATH_LENGTH);
    strcpy(actual_path,config.base_path);
    strcat(actual_path,path);

    //Add to Concurrent paths
    pthread_mutex_lock(&file_locks_mutex);
    pthread_mutex_lock(&config.config_mutex);

    int i=config.num_paths+1;
    pthread_mutex_init(&file_locks[i].mutex, NULL);
    strcpy(file_locks[i].filename,path);
    file_locks[i].ref_count = 0;

    pthread_mutex_unlock(&config.config_mutex);
    pthread_mutex_unlock(&file_locks_mutex);

    if (strcmp(type, "FILE") == 0) { 
        //Add to config accessible paths
        pthread_mutex_lock(&config.config_mutex);
        config.num_paths++;
        strcpy(config.accessible_paths[config.num_paths],path);
        pthread_mutex_unlock(&config.config_mutex);

        // Create an empty file (original implementation)
        FILE* file = fopen(actual_path, "w");
        if (!file) {
            log_message("ERROR: Unable to create file");
            return -1;
        }
        fclose(file);
        
        log_message("File created");
        return 0;
    }
    else if (strcmp(type, "FOLDER") == 0) {
        pthread_mutex_lock(&config.config_mutex);
        config.num_paths++;
        strcpy(config.accessible_paths[config.num_paths],path);
        pthread_mutex_unlock(&config.config_mutex);

        // Create directory with 0755 permissions
        if (mkdir(actual_path, 0755) != 0) {
            log_message("ERROR: Unable to create folder");
            return -1;
        }
        log_message("Folder created");
        return 0;
    }
    else {
        log_message("ERROR: Unknown creation type");
        return -1;
    }   
}

// Handle DELETE request
int handle_client_delete_request(const char* type, const char* path) {
    // Log the Create attempt
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), "Delete Request:%s Path=%s",type,path);
    log_message(log_msg);

    //Tokenise path
    
    char* actual_path=malloc(sizeof(path)+MAX_PATH_LENGTH);
    strcpy(actual_path,config.base_path);
    strcat(actual_path,path);
    
    pthread_mutex_lock(&file_locks_mutex);
    pthread_mutex_lock(&config.config_mutex);
    for(int i=0;i<config.num_paths;i++){
        if(strcmp(config.accessible_paths[i],actual_path)==0){
            for(int j=i;j<config.num_paths-1;j++){
                strcpy(config.accessible_paths[j],config.accessible_paths[j+1]);
                
                //Deleting/Removing lock and shifting others
                pthread_mutex_lock(&file_locks[j].mutex);
                pthread_mutex_lock(&file_locks[j+1].mutex);
                strcpy(file_locks[j].filename,file_locks[j+1].filename);
                file_locks[j].ref_count=file_locks[j+1].ref_count;
                pthread_mutex_unlock(&file_locks[j+1].mutex);
                pthread_mutex_lock(&file_locks[j+1].mutex);
            }
            config.num_paths--;
            break;
        }
    }
    pthread_mutex_unlock(&config.config_mutex);
    pthread_mutex_unlock(&file_locks_mutex);

    //Check
    if (type == NULL || actual_path == NULL) {
        log_message("ERROR: Invalid delete request");
        return -1;
    }

    if (strcmp(type, "FILE") == 0) {
        
        // Delete a file
        if (remove(actual_path)== 0) {
            log_message("File Deleted");
            return 0;
        }
        log_message("ERROR: Unable to Delete file");
        return -1;
    }
    else if (strcmp(type, "FOLDER") == 0) {
        // Delete a directory
        char command[1024];
        snprintf(command, sizeof(command), "rm -rf \"%s\"", path);
        int result = system(command);

        if (result == 0) {
            log_message("Folder Deleted");
            return 0;
        }
        log_message("ERROR: Unable to DELETE folder");
        return -1;
        
    }
    else {
        log_message("ERROR: Unknown Deletion type");
        return -1;
    }
}

// Handle INFO request
int handle_client_info_request(const char* upath, char* info_buffer) {
    struct stat file_stat;

    //Tokenise path
    char* path=malloc(sizeof(upath)+MAX_PATH_LENGTH);
    strcpy(path,config.base_path);
    strcat(path,upath);

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
int handle_client_stream_request(const char* upath, int client_socket) {
    //Tokenise path
    char* path=malloc(sizeof(upath)+MAX_PATH_LENGTH);
    strcpy(path,config.base_path);
    strcat(path,upath);

    FILE* audio_file = fopen(path, "rb");
    if (!audio_file) {
        int ack = htonl(FAILED);
        send(client_socket,&ack,sizeof(ack), 0);
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
    pthread_mutex_lock(&config.config_mutex);
    char response[BUFFER_SIZE] = "Accessible Paths:\n";
    for (int i = 0; i < config.num_paths; i++) {
        strcat(response, config.accessible_paths[i]);
        strcat(response, "\n");
    }
    pthread_mutex_unlock(&config.config_mutex);
    send(client_socket, response, strlen(response), 0);
    
    return 0;
}

// Zip function using system command
// Function to zip a path (file or folder) and send data to client
int zip_path_and_send(const char* source_path) {
    // Generate unique temporary zip filename
    char zip_path[MAX_PATH_LENGTH];
    snprintf(zip_path, sizeof(zip_path), "/tmp/zip_%ld.zip", time(NULL));

    // Construct zip command based on whether it's a file or directory
    char command[2*MAX_PATH_LENGTH];
    struct stat path_stat;

    // Validate source path[Not Doing]
    if (stat(source_path, &path_stat) != 0) {
        int ack = htonl(NOT_FOUND);
        send(sock,&ack,sizeof(ack), 0);
        return -1;
    }

    // Determine zip command based on path type
    if (S_ISDIR(path_stat.st_mode)) {
        // Zip directory recursively
        snprintf(command, sizeof(command),"zip -r '%s' '%s' > /dev/null 2>&1",zip_path, source_path);
    } else if (S_ISREG(path_stat.st_mode)) {
        // Zip single file
        snprintf(command, sizeof(command),"zip '%s' '%s' > /dev/null 2>&1",zip_path, source_path);
    } else {
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        return -1;
    }

    // Execute zip command
    int zip_result = system(command);
    if (zip_result != 0) {
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        return -1;
    }

    // Open the zip file
    FILE* zip_file = fopen(zip_path, "rb");
    if (!zip_file) {
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        return -1;
    }

    // Get file size
    fseek(zip_file, 0, SEEK_END);
    long file_size = ftell(zip_file);
    rewind(zip_file);

    // Allocate buffer for file contents
    char* file_buffer = malloc(file_size);
    if (!file_buffer) {
        fclose(zip_file);
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        return -1;
    }

    // Read file contents
    size_t read_size = fread(file_buffer, 1, file_size, zip_file);
    fclose(zip_file);

    if (read_size != file_size) {
        free(file_buffer);
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        return -1;
    }

    // Send file size first
    char size_buffer[32];
    snprintf(size_buffer, sizeof(size_buffer), "%ld", file_size);
    send(sock, size_buffer, strlen(size_buffer), 0);

    // Wait for client acknowledgment
    int response;
    recv(sock, &response, sizeof(response), 0);
    response = ntohl(response);

    // Send file contents
    ssize_t sent_bytes = send(sock, file_buffer, file_size, 0);

    // Clean up
    free(file_buffer);
    remove(zip_path);

    // Check if entire file was sent
    if (sent_bytes != file_size) {
        return -1;
    }
    return 0;
}

// Function to unzip received data to a destination
int unzip_received_data(const char* input) {
    char* copy_path = strdup(input);
    char* destination_path= strtok(copy_path, " ");
    char* data= strtok(NULL, "\0");

    // Ensure destination directory exists
    if (mkdir(destination_path, 0755) != 0 && errno != EEXIST) {
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        return -1;
    }

    // Generate unique temporary zip filename
    char zip_path[MAX_PATH_LENGTH];
    snprintf(zip_path, sizeof(zip_path), "/tmp/unzip_%ld.zip", time(NULL));

     // Write received zip data to temporary file
    FILE* zip_file = fopen(zip_path, "wb");
    if (!zip_file) {
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        log_message("ERROR: Unable to create temporary zip file");
        return -1;
    }
    
    size_t data_size = strlen(data);
    size_t written = fwrite(data, 1, data_size, zip_file);
    fclose(zip_file);

    if (written != data_size) {
        int ack = htonl(FAILED);
        send(sock,&ack,sizeof(ack), 0);
        log_message("ERROR: Unable to write zip data to temporary file");
        return -1;
    }

    // Unzip the file
    char unzip_command[2*MAX_PATH_LENGTH];
    snprintf(unzip_command, sizeof(unzip_command), 
             "unzip -o '%s' -d '%s' > /dev/null 2>&1", 
             zip_path, destination_path);
    
    int unzip_result = system(unzip_command);
    
    // Clean up temporary zip file
    remove(zip_path);

    if (unzip_result != 0) {
        log_message("ERROR: Unzip operation failed");
        return -1;
    }

    // Log successful unzip
    char log_msg[256];
    snprintf(log_msg, sizeof(log_msg), 
             "UNZIP SUCCESS: Destination=%s, Data Size=%zu", 
             destination_path, data_size);
    log_message(log_msg);

    // Update accessible paths
    pthread_mutex_lock(&config.config_mutex);
    if (config.num_paths < MAX_ACCESSIBLE_PATHS) {
        snprintf(config.accessible_paths[config.num_paths], 
                 MAX_PATH_LENGTH, 
                 "%s", destination_path);
        config.num_paths++;
    }
    pthread_mutex_unlock(&config.config_mutex);

    int ack = htonl(ACK);
    send(sock,&ack,sizeof(ack), 0);

    return 0;
}

// Function to handle client requests with lock support
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
    log_message(buffer);
    char* operation = strtok(buffer, " ");
    char* path = strtok(NULL, " ");  // For CREATE/DELETE/COPY It will be FILE/FOLDER

    if (path[0] == '/') path = path+1;

    // Handle different operations
    if (operation == NULL) {
        int ack = htonl(FAILED);
        send(client_sock,&ack,sizeof(ack), 0);
        return NULL;
    } 
    else if (strcmp(operation, "READ") == 0) {
        FileLock* lock = get_file_lock(path);
        if(lock==NULL){
            perror("Can't lock file");
            return NULL;
        }
        pthread_mutex_lock(&lock->mutex);
        lock->ref_count++;
        release_file_lock(lock);
        
        //Handling Acknowlegement
        if (handle_client_read_request(path, client_sock) == 0) {
            int ack = htonl(ACK);
            send(client_sock,&ack,sizeof(ack), 0);
        } 
        else {
            int ack = htonl(NOT_FOUND);
            send(client_sock,&ack,sizeof(ack), 0);
        }

        pthread_mutex_lock(&lock->mutex);
        lock->ref_count--;
        release_file_lock(lock);
    } 
    else if (strcmp(operation, "WRITE") == 0) {
        char* data = strtok(NULL, "");  // Get entire remaining data
        printf("joined monster = %s %s\n",path,data);
        

        //Get Lock and Do not unlock Write complete
        FileLock* lock = get_file_lock(path);
        if(lock==NULL){
            perror("Can't lock file");
            return NULL;
        }

        // char* data = strtok(NULL, "\0");  // Get entire remaining data
        if (data) {
            // Get data size
            size_t data_size = strlen(data);
            
            int sync=0;
            const char *substring = "SYNC";
            if (strstr(path, substring) != NULL) {
                sync=1;
            }

            //Write data to file
            if (data_size >= ASYNC_LIMIT && sync==0) {              // Asynchronous Write
                int ack = htonl(ASYNCHRONOUS_COMPLETE);
                send(client_sock,&ack,sizeof(ack), 0);

                if (handle_client_write_request(path, data) != 0) {
                    int ack = htonl(FAILED);
                    send(sock,&ack,sizeof(ack), 0);

                    //Release Lock
                    lock->ref_count--;
                    release_file_lock(lock);
                    return NULL;
                }
                ack = htonl(ASYNCHRONOUS_COMPLETE);
                send(sock,&ack,sizeof(ack), 0);
            }
            else {      
                if (handle_client_write_request(path, data) != 0) {  // Synchronous Write
                    int ack = htonl(FAILED);
                    send(client_sock,&ack,sizeof(ack), 0);
                    
                    //Release Lock
                    lock->ref_count--;
                    release_file_lock(lock);
                    return NULL;
                }
                int ack = htonl(ACK);
                send(client_sock,&ack,sizeof(ack), 0);  
            }
        } 
        else {
            int ack = htonl(FAILED);
            send(client_sock,&ack,sizeof(ack), 0);
        }

        //Release Lock
        lock->ref_count--;
    } 
    else if (strcmp(operation, "CREATE") == 0) {
        printf("%s",path); //Path is type
        char* upath = strtok(NULL," ");
        if (upath[0] == '/') upath = upath+1;
        
        if (handle_client_create_request(path,upath) == 0) {
            int ack = htonl(ACK);
            send(client_sock,&ack,sizeof(ack), 0);
        } else {
            int ack = htonl(FAILED);
            send(client_sock,&ack,sizeof(ack), 0);
        }
    }
    else if (strcmp(operation, "DELETE") == 0) {
        char* upath = strtok(NULL, " ");
        if (upath[0] == '/') upath = upath+1;
        
        FileLock* lock = get_file_lock(upath);
        if(lock==NULL){
            perror("Can't lock file");
            return NULL;
        }
        // while(1){
        pthread_mutex_lock(&lock->mutex);
        //     if(lock->ref_count==0){
        //         break;
        //     }
            // pthread_mutex_unlock(&lock->mutex);
        // }
        if (handle_client_delete_request(path,upath) == 0) {
            int ack = htonl(ACK);
            send(client_sock,&ack,sizeof(ack), 0);
            log_message("Delete Success");
        } else {
            int ack = htonl(FAILED);
            send(client_sock,&ack,sizeof(ack), 0);
        }
        pthread_mutex_unlock(&lock->mutex);
    } 
    else if (strcmp(operation, "INFO") == 0) {
        char info_buffer[BUFFER_SIZE];
        if (handle_client_info_request(path, info_buffer) == 0) {
            send(client_sock, info_buffer, strlen(info_buffer), 0);
            int ack = htonl(ACK);
            send(client_sock,&ack,sizeof(ack), 0);
        } 
        else {
            int ack = htonl(NOT_FOUND);
            send(client_sock,&ack,sizeof(ack), 0);
        }
    } 
    else if (strcmp(operation, "STREAM") == 0) {
        if (handle_client_stream_request(path, client_sock) == 0) {
            int ack = htonl(ACK);
            send(client_sock,&ack,sizeof(ack), 0);
        } else {
            int ack = htonl(NOT_FOUND);
            send(client_sock,&ack,sizeof(ack), 0);
        }
    }
    else if (strcmp(operation, "LIST") == 0) {
        handle_client_list_request(client_sock);
        int ack = htonl(ACK);
        send(client_sock,&ack,sizeof(ack), 0);
    }
    else if (strcmp(operation, "ZIP") == 0) {
        char* path = strtok(NULL, "\0");  // Rest of the buffer is the path
        
        // Call zip function
        if (zip_path_and_send(path) == 0) {
            int ack = htonl(ACK);
            send(client_sock,&ack,sizeof(ack), 0);
        } else {
            int ack = htonl(FAILED);
            send(client_sock,&ack,sizeof(ack), 0);
        }
        
    } 
    else if (strcmp(operation, "UNZIP") == 0) {
        char * input = strtok(NULL, " ");  // Get the destination path

        // Call unzip function
        if (unzip_received_data(input) == 0) {
            int ack = htonl(ACK);
            send(client_sock,&ack,sizeof(ack), 0);
        } else {
            int ack = htonl(FAILED);
            send(client_sock,&ack,sizeof(ack), 0);
        }
    }
    else {
        int ack = htonl(FAILED);
        send(client_sock,&ack,sizeof(ack), 0);
    }

    close(client_sock);
    free(client_socket_ptr);
    return NULL;
}