#include "lru.h"

typedef struct node {
    StorageServer *ss; // Pointer to the storage server
    char path[PATH_SIZE]; // Path associated with the storage server
    int num; // Index in the cache (optional)
    struct node *next; // Pointer to the next node
    struct node *prev; // Pointer to the previous node
} node;

node *head = NULL; // Head of the linked list
node *tail = NULL; // Tail of the linked list

// Function to initialize the cache
void cache_init() {
    head = malloc(sizeof(node));
    if (!head) {
        print_error("Failed to allocate memory for cache");
        exit(EXIT_FAILURE);
    }

    head->ss = NULL;
    head->num = 0;
    head->next = NULL;
    head->prev = NULL;

    node *temp = head;

    for (int i = 1; i < CACHE_SIZE; i++) {
        temp->next = malloc(sizeof(node));
        if (!temp->next) {
            print_error("Failed to allocate memory for cache node");
            exit(EXIT_FAILURE);
        }

        temp->next->prev = temp;
        temp = temp->next;

        temp->ss = NULL;
        temp->num = i;
        temp->next = NULL;
    }

    tail = temp; // Tail points to the last node
    // Make it circular
    tail->next = head;
    head->prev = tail;
}

// Helper function to move a node to the head of the cache
void move_to_head(node *n) {
    if (n == head) {
        return; // Already at the head
    }

    // Remove the node from its current position
    n->prev->next = n->next;
    n->next->prev = n->prev;

    // Attach it to the head
    n->next = head;
    n->prev = tail;

    tail->next = n;
    head->prev = n;

    head = n; // Update head pointer
}

// Function to search for a path in the storage servers
StorageServer* search_path(char *path) {    

    pthread_mutex_lock(&server_mutex);
    for (int i = 0; i < server_count; i++) {
        if (search_path_trie(storage_servers[i].paths_root, path)) {
            pthread_mutex_unlock(&server_mutex);
            return &storage_servers[i];
        }
    }

    pthread_mutex_unlock(&server_mutex);
    return NULL;
}

StorageServer* cache_search(char *path);

// Function to search for a path and insert if missing
StorageServer* cache_search_insert(char *path) {
    // Check if the path is already in the cache
    StorageServer *ss = cache_search(path);
    if (ss != NULL) {
        return ss; // Path already cached
    }

    // Translate the path to a StorageServer *
    ss = search_path(path);

    // Add to the head of the cache
    node *new_node = tail; // Reuse the least recently used node
    tail = tail->prev; // Update tail pointer
    tail->next = head; // Update circular connection
    head->prev = tail;

    // Update the new head node
    new_node->ss = ss;
    strncpy(new_node->path, path, PATH_SIZE);
    new_node->path[PATH_SIZE - 1] = '\0'; // Ensure null termination

    move_to_head(new_node);

    return ss;
}

// Function to search for a path in the cache
StorageServer* cache_search(char *path) {
    node *temp = head;

    // Traverse the cache
    do {
        if (temp->ss != NULL && strcmp(temp->path, path) == 0) {
            // Move the node to the head (most recently used)
            move_to_head(temp);
            return temp->ss;
        }
        temp = temp->next;
    } while (temp != head);

    // If not found, insert into the cache
    return NULL;
}

// Cleanup function to free all allocated memory
void cache_cleanup() {
    if (!head) {
        return;
    }

    node *temp = head;

    do {
        node *next = temp->next;
        free(temp);
        temp = next;
    } while (temp != head);

    head = NULL;
    tail = NULL;
}