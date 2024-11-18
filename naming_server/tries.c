#include "tries.h"

// Function to create a new Trie node
TrieNode *create_node(const char *path_component) {
    TrieNode *node = (TrieNode *)malloc(sizeof(TrieNode));
    node->path_component = strdup(path_component);
    node->next_sibling = NULL;
    node->first_child = NULL;
    return node;
}

// Helper function to find a child node with a specific path component
TrieNode *find_child(TrieNode *parent, const char *path_component) {
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
void insert_path_trie(TrieNode *root, const char *path) {
    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);

    char *token = strtok(path_copy, "/");
    TrieNode *current = root;

    while (token != NULL) {
        TrieNode *child = find_child(current, token);
        if (child == NULL) {
            // Create a new child node if not found
            child = create_node(token);
            child->next_sibling = current->first_child;
            current->first_child = child;
        }
        current = child;
        token = strtok(NULL, "/");
    }
}

// Function to search for a path in the Trie
int search_path_trie(TrieNode *root, const char *path) {
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

// Recursive function to delete a node and its children if needed
int delete_path_recursive(TrieNode *current, const char *path) {
    if (current == NULL) {
        return 0;
    }

    if (*path == '\0') {
        // Check if current has no children
        if (current->first_child == NULL) {
            free(current->path_component);
            free(current);
            return 1; // Node can be deleted
        }
        return 0;
    }

    char path_copy[strlen(path) + 1];
    strcpy(path_copy, path);
    char *token = strtok(path_copy, "/");

    TrieNode *parent = current;
    TrieNode *child = find_child(current, token);

    if (child != NULL && delete_path_recursive(child, strtok(NULL, ""))) {
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
void delete_path_trie(TrieNode *root, const char *path) {
    delete_path_recursive(root, path);
}