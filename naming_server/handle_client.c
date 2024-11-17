#include "handle_client.h"

// Function to search for a path in the storage servers
// server lock should be held
StorageServer* search_path(const char *path) {    

    for (int i = 0; i < server_count; i++) {
        if (strstr(storage_servers[i].accessible_paths, path) != NULL) {
            return &storage_servers[i];
        }
    }

    return NULL;
}

// NULL ss means path not found
// Server lock must be held if ss is not NULL
void send_ss_to_client(StorageServer* ss, int client_fd) {

    ClientResponse* response = (ClientResponse*) malloc(sizeof(ClientResponse)); 
    response->status = NOT_FOUND;

    if (ss) {

        response->status = OK;
        strcpy(response->server_ip, ss->ip);
        response->server_port = ss->client_port;

    }

    // Send the ClientResponse struct directly to the client
    if (send(client_fd, response, sizeof(ClientResponse), 0) < 0) {

        print_error("Error sending response to client");

    }

    if (ss) printf("Sent response to client: IP %s and port %d with status %d\n", response->server_ip, response->server_port, response->status);
    else printf("Sent response to client: Status %d\n", response->status);

    free(response);

}

void send_req_to_ss(StorageServer* ss, char* request) {

    int ss_fd = ss->fd;

    if (send(ss_fd, request, strlen(request), 0) < 0) {

        print_error("Error sending request to server");
        return;

    }

    printf("Forwarded request to server with IP %s and port %d\n", ss->ip, ss->nm_port);

}

// Function to handle read requests
void handle_read_request(int client_fd, char *file_path) {
    
    pthread_mutex_lock(&server_mutex);
    StorageServer* ss = search_path(file_path);
    send_ss_to_client(ss, client_fd);
    pthread_mutex_unlock(&server_mutex);

}

// Function to handle write requests
void handle_write_request(int client_fd, char *file_path) {
    
    pthread_mutex_lock(&server_mutex);
    StorageServer* ss = search_path(file_path);
    send_ss_to_client(ss, client_fd);
    pthread_mutex_unlock(&server_mutex);

}

// Function to handle file information requests
void handle_info_request(int client_fd, char *file_path) {
    
    pthread_mutex_lock(&server_mutex);
    StorageServer* ss = search_path(file_path);
    send_ss_to_client(ss, client_fd);
    pthread_mutex_unlock(&server_mutex);

}

// Function to handle audio streaming requests
void handle_stream_request(int client_fd, char *file_path) {
    
    pthread_mutex_lock(&server_mutex);
    StorageServer* ss = search_path(file_path);
    send_ss_to_client(ss, client_fd);
    pthread_mutex_unlock(&server_mutex);

}

// Function to handle create requests
void handle_create_request(int client_fd, char* request, char *path) {

    char* file_path;
    if (strncmp(path, "FILE", 4) == 0) file_path = path + 5;
    else file_path = path + 7;
    
    StorageServer* ss = NULL;
    char* last_slash = strrchr(file_path, '/');

    if (last_slash != NULL) {

        int length = last_slash - file_path;
        char *subpath = malloc(length+1);
        strncpy(subpath, file_path, length);
        subpath[length] = '\0';

        pthread_mutex_lock(&server_mutex);
        ss = search_path(subpath);
        pthread_mutex_unlock(&server_mutex);

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

    if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending acknowledgment to client");

}

// Function to handle delete requests
void handle_delete_request(int client_fd, char* request, char *path) {

    char* file_path;
    if (strncmp(path, "FILE", 4) == 0) file_path = path + 5;
    else file_path = path + 7;
    
    pthread_mutex_lock(&server_mutex);
    StorageServer* ss = search_path(file_path);
    pthread_mutex_unlock(&server_mutex);

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

    if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending acknowledgment to client");

}

// Function to handle copy requests between SS
void handle_copy_request(int client_fd, char *paths) {
    // Parse paths and facilitate the copying process between storage servers
}

// Function to handle listing all accessible paths
void handle_list_request(int client_fd) {

    pthread_mutex_lock(&server_mutex);

    for (int i = 0; i < server_count; i++) {

        StorageServer* ss = &storage_servers[i];
        
        char buffer[PATH_SIZE*MAX_PATHS_PER_SERVER];
        strcpy(buffer, ss->accessible_paths);
        
        if (send(client_fd, buffer, strlen(buffer), 0) < 0) {

            print_error("Error sending accessible paths to client");
            break;

        }

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
    char response[BUFFER_SIZE];
    int bytes_read;

    // Read client request
    bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {

        print_error("Error reading from client");
        close(client_fd);
        return NULL;

    }
    buffer[bytes_read-1] = '\0';

    printf("Received request from client: %s\n", buffer);

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
        write(client_fd, "Unknown command", strlen("Unknown command"));
    }

    close(client_fd);
    return NULL;
}