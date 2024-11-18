#include "tries.h"

// Function to create a new Trie node
TrieNode *create_node(char *path_component, bool isFile) {

    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    node->path_component = strdup(path_component);
    node->next_sibling = NULL;
    node->first_child = NULL;
    node->isFile = isFile;

    return node;
}

// Helper function to find a child node with a specific path component
TrieNode *find_child(TrieNode *parent, char *path_component) {
    TrieNode *current = parent->first_child;
    while (current != NULL) {
        if (strcmp(current->path_component, path_component) == 0) {
            return current;
        }
        current = current->next_sibling;
    }
    return NULL;
}

// Function to insert a path into the Trie
void insert_path_trie(TrieNode *root, char *path) {

    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);

    bool isFile = true;
    if (path_copy[strlen(path_copy)-1] == '/') {
        
        isFile = false;
        path_copy[strlen(path_copy)-1] = '\0';

    }

    char *token = strtok(path_copy, "/");
    TrieNode *current = root;

    while (token != NULL) {
        TrieNode *child = find_child(current, token);

        char* path_component = strdup(token);
        token =  strtok(NULL, "/");

        if (child == NULL && token == NULL) {

            // Create a new child node if not found
            if (isFile) child = create_node(path_component, true);
            else child = create_node(path_component, false);

            child->next_sibling = current->first_child;
            current->first_child = child;

            free(path_component);
            return;
        }

        free(path_component);
        current = child;
    }
}

// Function to search for a path in the Trie
bool search_path_trie(TrieNode *root, char *path) {
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);

    char *token = strtok(path_copy, "/");
    TrieNode *current = root;

    while (token != NULL) {
        current = find_child(current, token);
        if (current == NULL) {
            return 0; // Path not found
        }
        token = strtok(NULL, "/");
    }
    return current != NULL;
}

// Recursive function to delete a node and its children
void delete_all_children(TrieNode *node) {
    if (node == NULL) {
        return;
    }

    TrieNode *child = node->first_child;
    while (child != NULL) {
        TrieNode *next_sibling = child->next_sibling;
        delete_all_children(child);
        free(child->path_component);
        free(child);
        child = next_sibling;
    }
}

// Recursive function to delete a node and its children if needed
int delete_path_helper(TrieNode *current, char *path, bool isFile) {

    if (*path == '\0' || strcmp(path, "/") == 0) {
        
        delete_all_children(current);
        return 1;

    }

    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    char *token = strtok(path_copy, "/");

    TrieNode *parent = current;
    TrieNode *child = find_child(current, token);

    // TODO: revise

    if (child != NULL && delete_path_helper(child, strtok(NULL, ""), isFile)) {
        // Remove the child from the parent's list
        if (parent->first_child == child) {
            parent->first_child = child->next_sibling;
        } else {
            TrieNode *sibling = parent->first_child;
            while (sibling->next_sibling != child) {
                sibling = sibling->next_sibling;
            }
            sibling->next_sibling = child->next_sibling;
        }
        free(child->path_component);
        free(child);
        return parent->first_child == NULL;
    }

    return 0;
}

// Function to delete a path from the Trie
void delete_path_trie(TrieNode *root, char *path) {

    bool isFile = true;
    if (path[strlen(path)-1] = '/') isFile = false;

    delete_path_recursive(root, path, isFile);
}

// Recursive function to store all paths in a 2D buffer
void store_all_paths(TrieNode *node, char buffer[], int depth, char paths[][PATH_SIZE], int *path_index) {
    if (node == NULL) {
        return;
    }

    // Append the current path component to the buffer
    strcat(buffer, node->path_component);
    if (!node->isFile) {
        strcat(buffer, "/");
    }

    strcpy(paths[*path_index], buffer);
    (*path_index)++;

    // Recurse on the first child
    store_all_paths(node->first_child, buffer, depth + 1, paths, path_index);

    // Remove the current component from the buffer for backtracking
    buffer[strlen(buffer) - strlen(node->path_component) - (!node->isFile ? 1 : 0)] = '\0';

    // Recurse on the next sibling
    store_all_paths(node->next_sibling, buffer, depth, paths, path_index);
}

// Wrapper function to start storing paths from the root
void store_all_paths_trie(TrieNode *root, char paths[][PATH_SIZE], int *path_count) {
    char buffer[PATH_SIZE] = "";
    *path_count = 0;
    store_all_paths(root->first_child, buffer, 0, paths, path_count);
}