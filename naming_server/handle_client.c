#include "handle_client.h"

// NULL ss means path not found
void send_ss_to_client(StorageServer* ss, int client_fd) {

    ClientResponse* response = (ClientResponse*) malloc(sizeof(ClientResponse)); 
    response->status = NOT_FOUND;

    if (ss) {

        pthread_mutex_lock(&server_mutex);
        response->status = OK;
        strcpy(response->server_ip, ss->ip);
        response->server_port = ss->client_port;
        pthread_mutex_unlock(&server_mutex);

    }

    // Send the ClientResponse struct directly to the client
    if (send(client_fd, response, sizeof(ClientResponse), 0) < 0) {

        print_error("Error sending response to client");
        free(response);
        return;

    }

    char message[BUFFER_SIZE];

    if (ss) {

        sprintf(message, "Sent response to client %d: IP %s and port %d with status %d\n", client_fd, response->server_ip, response->server_port, response->status);
        log_message(message);

    }
    else {

        sprintf(message, "Sent response to client %d: Status %d\n", client_fd, response->status);
        log_message(message);

    }

    free(response);

}

void send_req_to_ss(StorageServer* ss, char* request) {

    pthread_mutex_lock(&server_mutex);
    int ss_fd = ss->fd;
    pthread_mutex_unlock(&server_mutex);

    if (send(ss_fd, request, strlen(request), 0) < 0) {

        print_error("Error sending request to server");
        return;

    }

    printf("Forwarded request to server with IP %s and port %d\n", ss->ip, ss->nm_port);

}

// Function to handle read requests
void handle_read_request(int client_fd, char *file_path) {
    
    StorageServer* ss = cache_search_insert(file_path);
    send_ss_to_client(ss, client_fd);

}

// Function to handle write requests
void handle_write_request(int client_fd, char *remaining_command) {

    bool sync = false;
    if (strstr(remaining_command, "-SYNC") != NULL) sync = true;
    
    char* file_path = strtok(remaining_command, " ");
    if (sync) file_path = strtok(NULL, " ");
    char* data = strtok(NULL, "");

    StorageServer* ss = cache_search_insert(file_path);
    send_ss_to_client(ss, client_fd);

    if (strlen(data) < ASYNC_LIMIT || sync) return;

    int ack;
    if (recv(ss->fd, &ack, sizeof(ack), 0) < 0) {

        print_error("reading async ack failed");
        return;

    }

    if (send(client_fd, &ack, sizeof(ack), 0) < 0) {

        print_error("sending async ack failed");

    }

}

// Function to handle file information requests
void handle_info_request(int client_fd, char *file_path) {
    
    StorageServer* ss = cache_search_insert(file_path);
    send_ss_to_client(ss, client_fd);

}

// Function to handle audio streaming requests
void handle_stream_request(int client_fd, char *file_path) {
    
    StorageServer* ss = cache_search_insert(file_path);
    send_ss_to_client(ss, client_fd);

}

// Function to handle create requests
void handle_create_request(int client_fd, char* request, char *path) {

    char* file_path = (char*) malloc(sizeof(char)*strlen(path));
    if (strncmp(path, "FILE", 4) == 0) strcpy(file_path, path + 5);
    else {
        
        strcpy(file_path, path + 7);
        if (file_path[strlen(file_path)-1] == '/') file_path[strlen(file_path)-1] = '\0';

    }

    printf("%s\n", file_path);
    
    StorageServer* ss = NULL;
    char* subpath = str_before_last_slash(file_path);

    printf("%s\n", subpath);

    if (subpath != NULL) {

        ss = cache_search_insert(subpath);

        free(subpath);
        
    }

    if (ss == NULL) {
        
        send_ss_to_client(NULL, client_fd);
        return;

    }
    
    send_req_to_ss(ss, request);

    int status;
    int bytes_received = recv(ss->fd, &status, sizeof(status), 0);

    if (bytes_received < 0) {

        print_error("Error receiving acknowledgment from storage server");
        return;

    }

    status = ntohl(status);

    if (status == ACK) insert_path_trie(ss->paths_root, file_path);
    free(file_path);

    status = htonl(status);

    if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending acknowledgment to client");

}

// Function to handle delete requests
void handle_delete_request(int client_fd, char* request, char *path) {

    char* file_path;
    if (strncmp(path, "FILE", 4) == 0) file_path = path + 5;
    else file_path = path + 7;
    
    StorageServer* ss = cache_search_insert(file_path);

    if (ss == NULL) {
        
        send_ss_to_client(NULL, client_fd);
        return;

    }

    send_req_to_ss(ss, request);

    int status;
    int bytes_received = recv(ss->fd, &status, sizeof(status), 0);

    if (bytes_received < 0) {

        print_error("Error receiving acknowledgment from storage server");
        return;

    }

    status = ntohl(status);

    if (status == ACK) delete_path_trie(ss->paths_root, file_path);

    status = htonl(status);

    if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending acknowledgment to client");

}

// Function to handle copy requests between storage servers via the naming server
void handle_copy_request(int client_fd, char *paths) {

    //TODO: copy folder, add accessible paths

    char* file_path;
    if (strncmp(paths, "FILE", 4) == 0) file_path = paths + 5;
    else file_path = paths + 7;

    // Parse paths to extract source and destination paths
    char *source_path = strtok(paths, " ");
    char *destination_path = strtok(NULL, " ");

    if (source_path == NULL || destination_path == NULL) {

        int status = htonl(NOT_FOUND);
        if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending status to client");
        return;

    }

    // Find source and destination storage servers
    StorageServer *source_ss = NULL;
    StorageServer *destination_ss = NULL;

    source_ss = cache_search_insert(source_path);
    char* destination_subpath = str_before_last_slash(destination_path);
    if (destination_subpath != NULL) {
        
        destination_ss = cache_search_insert(destination_subpath);
        free(destination_subpath);

    }

    if (source_ss == NULL || destination_ss == NULL) {
        
        int status = htonl(NOT_FOUND);
        if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending status to client");
        return;

    }

    // Send a read request to the source storage server
    char read_request[BUFFER_SIZE];
    snprintf(read_request, BUFFER_SIZE, "READ %s", source_path);
    send_req_to_ss(source_ss, read_request);

    // Send a create request to the destination storage server
    char create_request[BUFFER_SIZE];
    snprintf(create_request, BUFFER_SIZE, "CREATE FILE %s", destination_path);
    send_req_to_ss(source_ss, create_request);

    // Receive status from the destination storage server
    int status;
    if (recv(destination_ss->fd, &status, sizeof(status), 0) < 0) {

        print_error("Error receiving status from destination storage server");
        return;

    }

    status = ntohl(status);

    if (status != OK) {

        status = htonl(status);
        if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending status to client");
        return;

    }

    // Send a write request to the destination storage server
    char write_request[BUFFER_SIZE];
    snprintf(write_request, BUFFER_SIZE, "WRITE %s", destination_path);
    send_req_to_ss(destination_ss, write_request);

    // Loop to read from the source and write to the destination
    char data_buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(source_ss->fd, data_buffer, sizeof(data_buffer), 0)) > 0) {

        // Send the data chunk to the destination
        if (send(destination_ss->fd, data_buffer, bytes_received, 0) < 0) {
            print_error("Error sending data to destination storage server");
            return;
        }

    }

    if (bytes_received < 0) {
        print_error("Error receiving data from source storage server");
        return;
    }

    // Receive status from the destination storage server
    if (recv(destination_ss->fd, &status, sizeof(status), 0) < 0) {
        print_error("Error receiving acknowledgment from destination storage server");
        return;
    }

    // Send acknowledgment back to the client
    if (send(client_fd, &status, sizeof(status), 0) < 0) {
        print_error("Error sending acknowledgment to client");
    }

}

// Function to handle listing all accessible paths
void handle_list_request(int client_fd) {

    pthread_mutex_lock(&server_mutex);

    for (int i = 0; i < server_count; i++) {

        StorageServer* ss = &storage_servers[i];
        
        int count;
        char buffer[MAX_PATHS_PER_SERVER][PATH_SIZE];
        store_all_paths_trie(ss->paths_root, buffer, &count);

        bool error = false;

        for (int i = 0; i < count; i++) {

            char* path = buffer[i];
            strcat(path, "\n");
        
            if (send(client_fd, path, strlen(path), 0) < 0) {

                print_error("Error sending accessible paths to client");
                error = true;
                break;

            }

        }

        if (error) break;

    }

    pthread_mutex_unlock(&server_mutex);

    // Signal end of list to the client
    if (send(client_fd, "STOP", strlen("STOP"), 0) < 0) {
        print_error("Error sending STOP to client for list");
    }

}

/* MAIN FUNCTION */

// Function to handle client requests
void *client_handler(void *client_sock_fd) {

    int client_fd = *(int *)client_sock_fd;
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Read client request
    bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {

        print_error("Error reading from client");
        close(client_fd);
        return NULL;

    }
    buffer[bytes_read-1] = '\0';

    char message[BUFFER_SIZE+100];
    sprintf(message, "Received request from client: %s\n\n", buffer);
    log_message(message);

    /*
    
    REQUEST FORMATS

    - READ <path>
    - WRITE <path>
    - INFO <path>
    - STREAM <path>
    - CREATE FILE/FOLDER <path>
    - DELETE FILE/FOLDER <path>
    - COPY FILE/FOLDER <path1> <path2>
    - LIST

    */

    // Determine request type
    if (strncmp(buffer, "READ", 4) == 0) {
        handle_read_request(client_fd, buffer + 5); // Pass the file path
    } else if (strncmp(buffer, "WRITE", 5) == 0) {
        handle_write_request(client_fd, buffer + 6); // Pass the file path
    } else if (strncmp(buffer, "INFO", 4) == 0) {
        handle_info_request(client_fd, buffer + 5); // Pass the file path
    } else if (strncmp(buffer, "STREAM", 6) == 0) {
        handle_stream_request(client_fd, buffer + 7); // Pass the file path
    } else if (strncmp(buffer, "CREATE", 6) == 0) {
        handle_create_request(client_fd, buffer, buffer + 7); // Pass the file path
    } else if (strncmp(buffer, "DELETE", 6) == 0) {
        handle_delete_request(client_fd, buffer, buffer + 7); // Pass the file path
    } else if (strncmp(buffer, "COPY", 4) == 0) {
        handle_copy_request(client_fd, buffer + 5); // Pass source and destination paths
    } else if (strncmp(buffer, "LIST", 4) == 0) {
        handle_list_request(client_fd);
    } else {
        if (send(client_fd, "Unknown command", strlen("Unknown command"), 0) < 0) print_error("sending invalid command error failed");
    }

    sprintf(message, "Client connection %d closed.\n\n", client_fd);
    log_message(message);

    close(client_fd);
    return NULL;
}