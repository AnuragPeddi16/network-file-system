#include "handle_client.h"

// Function to search for a path in the storage servers
ClientResponse* search_path(const char *path) {

    ClientResponse* response = (ClientResponse*) malloc(sizeof(ClientResponse)); 
    response->found = 0;

    pthread_mutex_lock(&server_mutex);
    for (int i = 0; i < server_count; i++) {
        if (strstr(storage_servers[i].accessible_paths, path) != NULL) {
            
            strcpy(response->server_ip, storage_servers[i].ip);
            response->server_port = storage_servers[i].client_port;
            
            pthread_mutex_unlock(&server_mutex);
            return response;
        }
    }
    
    pthread_mutex_unlock(&server_mutex);
    return response;
}

// Function to handle client requests
void *client_handler(void *client_sock_fd) {

    int sock_fd = *(int *)client_sock_fd;
    char buffer[BUFFER_SIZE];
    char response[BUFFER_SIZE];
    int bytes_read;

    // Read client request
    bytes_read = read(sock_fd, buffer, sizeof(buffer) - 1);
    if (bytes_read < 0) {

        print_error("Error reading from client");
        close(sock_fd);
        return;

    }
    buffer[bytes_read] = '\0';

    printf("Received request: %s\n", buffer);

    // Request format is <REQUEST> FILE/FOLDER(for CREATE, DELETE, COPY) <path>
    // eg: READ dir1/a.txt
    // eg: COPY FOLDER dir1/folder1

    // Determine request type
    if (strncmp(buffer, "READ", 4) == 0) {
        handle_read_request(sock_fd, buffer + 5); // Pass the file path
    } else if (strncmp(buffer, "WRITE", 5) == 0) {
        handle_write_request(sock_fd, buffer + 6); // Pass the file path
    } else if (strncmp(buffer, "INFO", 4) == 0) {
        handle_info_request(sock_fd, buffer + 5); // Pass the file path
    } else if (strncmp(buffer, "STREAM", 6) == 0) {
        handle_stream_request(sock_fd, buffer + 7); // Pass the file path
    } else if (strncmp(buffer, "CREATE", 6) == 0) {
        handle_create_request(sock_fd, buffer + 7); // Pass the file path
    } else if (strncmp(buffer, "DELETE", 6) == 0) {
        handle_delete_request(sock_fd, buffer + 7); // Pass the file path
    } else if (strncmp(buffer, "COPY", 4) == 0) {
        handle_copy_request(sock_fd, buffer + 5); // Pass source and destination paths
    } else if (strncmp(buffer, "LIST", 4) == 0) {
        handle_list_request(sock_fd);
    } else {
        write(sock_fd, "Unknown command", strlen("Unknown command"));
    }

    close(sock_fd);
    return;
}

// Function to handle read requests
void handle_read_request(int sock_fd, char *file_path) {
    
    ClientResponse* response = search_path(file_path);

    // Send the ClientResponse struct directly to the client
    if (send(sock_fd, response, sizeof(ClientResponse), 0) < 0) {
        print_error("Error sending response to client");
    }

}

// Function to handle write requests
void handle_write_request(int sock_fd, char *file_path) {
    
    ClientResponse* response = search_path(file_path);

    // Send the ClientResponse struct directly to the client
    if (send(sock_fd, response, sizeof(ClientResponse), 0) < 0) {
        print_error("Error sending response to client");
    }

}

// Function to handle file information requests
void handle_info_request(int sock_fd, char *file_path) {
    
    ClientResponse* response = search_path(file_path);

    // Send the ClientResponse struct directly to the client
    if (send(sock_fd, response, sizeof(ClientResponse), 0) < 0) {
        print_error("Error sending response to client");
    }

}

// Function to handle audio streaming requests
void handle_stream_request(int sock_fd, char *file_path) {
    
    ClientResponse* response = search_path(file_path);

    // Send the ClientResponse struct directly to the client
    if (send(sock_fd, response, sizeof(ClientResponse), 0) < 0) {
        print_error("Error sending response to client");
    }

}

// Function to handle create requests
void handle_create_request(int sock_fd, char *file_path) {
    
    
    
}

// Function to handle delete requests
void handle_delete_request(int sock_fd, char *file_path) {
    // Locate the SS and forward the delete request
}

// Function to handle copy requests between SS
void handle_copy_request(int sock_fd, char *paths) {
    // Parse paths and facilitate the copying process between storage servers
}

// Function to handle listing all accessible paths
void handle_list_request(int sock_fd) {
    // Retrieve and send a list of all accessible paths from registered storage servers
}
