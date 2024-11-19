#include "client.h"

int connect_to_server(const char *ip, int port)
{ 
    /*tries to connect to any server*/

    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_fd < 0)
    {
        perror("Socket creation failed");
        return -1;
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (inet_pton(AF_INET, ip, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address");
        close(sock_fd);
        return -1;
    }

    if (connect(sock_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Connection failed");
        close(sock_fd);
        return -1;
    }

    return sock_fd;
}

ServerInfo get_storage_server_info(int naming_server_fd, const char *path, int operation)
{
    ServerInfo server_info = {0};
    char request[BUFFER_SIZE];

    // Format request to naming server

    // added
    // if (operation == 1)
    // {
    //     snprintf(request, sizeof(request), "%s %s\n", "READ", path); // operation send the name directy not the code
    //     printf("Request: %s\n", request);                            // test
    // }
    // Prepare request based on operation type

    switch(operation) {
        case OP_READ:
            snprintf(request, sizeof(request), "READ %s\n", path);
            break;
        case OP_WRITE:
            snprintf(request, sizeof(request), "WRITE %s\n", path);
            break;
        case OP_CREATE:
            snprintf(request, sizeof(request), "CREATE %s\n", path);
            break;
        case OP_DELETE:
            snprintf(request, sizeof(request), "DELETE %s\n", path);
            break;
        case OP_LIST:
            snprintf(request, sizeof(request), "LIST %s\n", path);
            break;
        case OP_STREAM:
            snprintf(request, sizeof(request), "STREAM %s\n", path);
            break;
        case OP_COPY:
            snprintf(request, sizeof(request), "COPY %s\n", path);
            break;
        default:
            printf("Unknown operation\n");
            server_info.status = -1;
            return server_info;

    }

    // snprintf(request, sizeof(request), "%d %s", operation, path); //operation send the name directy not the code

    // Send request to naming server
    if (send(naming_server_fd, request, strlen(request), 0) < 0)
    {
        perror("Send failed");
        return server_info;
    }

    // Receive response
    ClientResponse response;
    ssize_t bytes_received = recv(naming_server_fd, &response, sizeof(response), 0);

    if (bytes_received < 0)
    {
        perror("Receive from naming server failed");
        server_info.status = -1;
        return server_info;
    }

    // Check if receive was successful

    // if (bytes_received != sizeof(response))
    // {

    //     printf("Incomplete response received\n");

    //     server_info.status = -1;

    //     return server_info;
    // }

    // Populate server_info with received response

    server_info.status = response.status;
    strncpy(server_info.ip, response.server_ip, INET_ADDRSTRLEN - 1);
    server_info.ip[INET_ADDRSTRLEN - 1] = '\0';
    server_info.port = response.server_port;

    // Debug print

    printf("Received response: Status=%d, IP=%s, Port=%d\n",server_info.status, server_info.ip, server_info.port);
    return server_info;
}

int handle_read_operation(const char *path)
{
    // Connect to naming server
    int naming_server_fd = connect_to_server("127.0.0.1", NAMING_SERVER_PORT);
    if (naming_server_fd < 0)
        return -1;

    // Get storage server info
    ServerInfo storage_server = get_storage_server_info(naming_server_fd, path, OP_READ);
    close(naming_server_fd);
    // printf()

    // if (storage_server.status != 0)
    // { // put other status code also
    //     printf("Failed to get storage server information\n");
    //     return -1;
    // }

    switch(storage_server.status) {
        case OK:
            printf(GREEN("STATUS CODE : OK\n"));
            break;
        case ACK:
            printf(YELLOW("STATUS CODE : ACK\n"));
            break;
        case NOT_FOUND:
            printf(RED("STATUS CODE : NOT_FOUND\n"));
            break;
        case FAILED:
            printf(MAGENTA("STATUS CODE : FAILED\n"));
            break;
        case ASYNCHRONOUS_COMPLETE:
            printf(CYAN("STATUS CODE : ASYNCHRONOUS_COMPLETE\n"));
            break;
        default:
            printf("Unknown operation\n");
            storage_server.status = -1;
            // return storage_server;

    }

    // Connect to storage server
    int storage_fd = connect_to_server(storage_server.ip, storage_server.port);
    if (storage_fd < 0)
        return -1;

    // Send read request
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "READ %s", path);
    send(storage_fd, request, strlen(request), 0);

    // Receive and display file content
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(storage_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "STOP") != NULL)
            break;
    }

    close(storage_fd);
    return 0;
}

int handle_write_operation(const char *path, const char *content)
{
    // Similar structure to read operation but sends content to storage server
    int naming_server_fd = connect_to_server("127.0.0.1", NAMING_SERVER_PORT);
    if (naming_server_fd < 0)
        return -1;

    ServerInfo storage_server = get_storage_server_info(naming_server_fd, path, OP_WRITE);
    close(naming_server_fd);

    // if (storage_server.port == 0)
    // {
    //     printf("Failed to get storage server information\n");
    //     return -1;
    // }
    switch(storage_server.status) {
        case OK:
            printf(GREEN("STATUS CODE : OK\n"));
            break;
        case ACK:
            printf(YELLOW("STATUS CODE : ACK\n"));
            break;
        case NOT_FOUND:
            printf(RED("STATUS CODE : NOT_FOUND\n"));
            break;
        case FAILED:
            printf(MAGENTA("STATUS CODE : FAILED\n"));
            break;
        case ASYNCHRONOUS_COMPLETE:
            printf(CYAN("STATUS CODE : ASYNCHRONOUS_COMPLETE\n"));
            break;
        default:
            printf("Unknown operation\n");
            storage_server.status = -1;
            // return storage_server;

    }

    int storage_fd = connect_to_server(storage_server.ip, storage_server.port);
    if (storage_fd < 0)
        return -1;

    // Send write request and content
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "WRITE %s\n%s", path, content);
    send(storage_fd, request, strlen(request), 0);

    // Wait for acknowledgment
    char response[BUFFER_SIZE] = {0};
    recv(storage_fd, response, sizeof(response) - 1, 0);

    close(storage_fd);
    return 0;
}

int handle_stream_operation(const char *path)
{
    int naming_server_fd = connect_to_server("127.0.0.1", NAMING_SERVER_PORT);
    if (naming_server_fd < 0)
        return -1;

    ServerInfo storage_server = get_storage_server_info(naming_server_fd, path, OP_STREAM);
    close(naming_server_fd);

    if (storage_server.port == 0)
    {
        printf("Failed to get storage server information\n");
        return -1;
    }

    int storage_fd = connect_to_server(storage_server.ip, storage_server.port);
    if (storage_fd < 0)
        return -1;

    // Send stream request
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "STREAM %s", path);
    send(storage_fd, request, strlen(request), 0);

    // Process audio stream (basic implementation)
    char buffer[BUFFER_SIZE];
    FILE *audio_pipe = popen("mpv -", "w");
    if (!audio_pipe)
    {
        perror("Failed to open audio player");
        close(storage_fd);
        return -1;
    }

    int bytes_received;
    while ((bytes_received = recv(storage_fd, buffer, sizeof(buffer), 0)) > 0)
    {
        fwrite(buffer, 1, bytes_received, audio_pipe);
        if (bytes_received < sizeof(buffer))
            break; // End of stream
    }

    pclose(audio_pipe);
    close(storage_fd);
    return 0;
}

//talk only to ns
int handle_create_operation(const char *path, const char *name)
{
    int naming_server_fd = connect_to_server("127.0.0.1", NAMING_SERVER_PORT);
    if (naming_server_fd < 0)
        return -1;

    ServerInfo storage_server = get_storage_server_info(naming_server_fd, path, OP_CREATE);

    if (storage_server.status == 1)
    {
        printf(GREEN("File/Folder created normally\n"));
    }
    else if (storage_server.status == 3)
    {
        printf(MAGENTA("File/Folder create failed\n"));
    }
    else
    {
        printf(RED("creating unknown error\n"));
    }

    
    
    close(naming_server_fd);

    // if (storage_server.port == 0)
    // {
    //     printf("Failed to get storage server information\n");
    //     return -1;
    // }

   

    // int storage_fd = connect_to_server(storage_server.ip, storage_server.port);
    // if (storage_fd < 0)
    //     return -1;

    // // Send create request
    // char request[BUFFER_SIZE];
    // snprintf(request, sizeof(request), "CREATE %s %s", path, name);
    // send(storage_fd, request, strlen(request), 0);

    // // Wait for acknowledgment
    // char response[BUFFER_SIZE] = {0};
    // recv(storage_fd, response, sizeof(response) - 1, 0);

    // close(storage_fd);
    // return (strstr(response, "SUCCESS") != NULL) ? 0 : -1;
    return 0;
}

int handle_delete_operation(const char *path)
{
    int naming_server_fd = connect_to_server("127.0.0.1", NAMING_SERVER_PORT);
    if (naming_server_fd < 0)
        return -1;

    ServerInfo storage_server = get_storage_server_info(naming_server_fd, path, OP_DELETE);
    close(naming_server_fd);

    if (storage_server.port == 0)
    {
        printf("Failed to get storage server information\n");
        return -1;
    }

    int storage_fd = connect_to_server(storage_server.ip, storage_server.port);
    if (storage_fd < 0)
        return -1;

    // Send delete request
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "DELETE %s", path);
    send(storage_fd, request, strlen(request), 0);

    // Wait for acknowledgment
    char response[BUFFER_SIZE] = {0};
    recv(storage_fd, response, sizeof(response) - 1, 0);

    close(storage_fd);
    return (strstr(response, "SUCCESS") != NULL) ? 0 : -1;
}

int handle_list_operation(const char *path)
{
    int naming_server_fd = connect_to_server("127.0.0.1", NAMING_SERVER_PORT);
    if (naming_server_fd < 0)
        return -1;

    ServerInfo storage_server = get_storage_server_info(naming_server_fd, path, OP_LIST);
    close(naming_server_fd);

    if (storage_server.port == 0)
    {
        printf("Failed to get storage server information\n");
        return -1;
    }

    int storage_fd = connect_to_server(storage_server.ip, storage_server.port);
    if (storage_fd < 0)
        return -1;

    // Send list request
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "LIST %s", path);
    send(storage_fd, request, strlen(request), 0);

    // Receive and display directory listing
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while ((bytes_received = recv(storage_fd, buffer, sizeof(buffer) - 1, 0)) > 0)
    {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        if (strstr(buffer, "STOP") != NULL)
            break;
    }

    close(storage_fd);
    return 0;
}

int handle_copy_operation(const char *source, const char *dest)
{
    int naming_server_fd = connect_to_server("127.0.0.1", NAMING_SERVER_PORT);
    if (naming_server_fd < 0)
        return -1;

    // For copy, we need to communicate only with naming server as it handles the copy operation
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request), "COPY %s %s", source, dest);
    send(naming_server_fd, request, strlen(request), 0);

    // Wait for acknowledgment
    char response[BUFFER_SIZE] = {0};
    recv(naming_server_fd, response, sizeof(response) - 1, 0);

    close(naming_server_fd);
    return (strstr(response, "SUCCESS") != NULL) ? 0 : -1;
}