    sprintf(message, "Received ack with status %d\n\n", status);
    log_message(message);

    if (status != ACK) {

        status = htonl(status);
        if (send(client_fd, &status, sizeof(status), 0) < 0) print_error("Error sending status to client");
        return;

    }

    insert_path_trie(destination_ss->paths_root, destination_path);
    if (!isFile) {
        
        TrieNode* source_node = search_path_trie(source_ss->paths_root, source_path);
        TrieNode* dest_node = search_path_trie(destination_ss->paths_root, destination_path);
        merge_trees(dest_node, source_node);

    }

    int bytes_received;

    if (isFile) {

        // Loop to read from the source and write to the destination
        char data_buffer[MAX_FILE_SIZE];
        bytes_received = recv(source_ss_fd, data_buffer, sizeof(data_buffer), 0); 

        char* ptr;
        if ((ptr = strstr(data_buffer, "STOP")) != NULL) *ptr = '\0';

        // Send a write request to the destination storage server
        char write_request[MAX_FILE_SIZE+PATH_SIZE+100];
        snprintf(write_request, MAX_FILE_SIZE+PATH_SIZE+100, "WRITE %s \"%s\"", destination_path, data_buffer);
        close(dest_ss_fd);
        dest_ss_fd = send_req_to_ss(destination_ss, write_request);

        

    } else {

        char data_buffer[MAX_FILE_SIZE];
        bytes_received = recv(source_ss_fd, data_buffer, sizeof(data_buffer), 0);
        if (bytes_received < 0) {

            print_error("copying write failed");
            return;

        }

        // Send a write request to the destination storage server
        char write_request[MAX_FILE_SIZE + PATH_SIZE + 100];
        snprintf(write_request, MAX_FILE_SIZE + PATH_SIZE + 100, "UNZIP %s \"%s\"", destination_path, data_buffer);
        close(dest_ss_fd);
        dest_ss_fd = send_req_to_ss(destination_ss, write_request);

    }

    if (bytes_received < 0) {
        print_error("Error receiving data from source storage server");
        return;
    }

    // Receive status from the destination storage server
    if (recv(dest_ss_fd, &status, sizeof(status), 0) < 0) {
        print_error("Error receiving acknowledgment from destination storage server");
        return;
    }

    close(source_ss_fd);
    close(dest_ss_fd);

    status = ntohl(status);

    sprintf(message, "Received ack with status %d\n\n", status);
    log_message(message);

    status = htonl(status);

    // Send acknowledgment back to the client
    if (send(client_fd, &status, sizeof(status), 0) < 0) {
        print_error("Error sending acknowledgment to client");
    }
