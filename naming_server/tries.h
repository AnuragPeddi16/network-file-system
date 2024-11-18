#ifndef _TRIES_H_
#define _TRIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define PATH_SIZE 4096

// Structure of a Trie node
typedef struct TrieNode {
    char *path_component;
    struct TrieNode *next_sibling;
    struct TrieNode *first_child;
    bool isFile;
} TrieNode;

TrieNode *create_node(const char *path_component, bool isFile);
void insert_path_trie(TrieNode *root, char *path);
bool search_path_trie(TrieNode *root, char *path);
void delete_path_trie(TrieNode *root, char *path);
void store_all_paths_trie(TrieNode *root, char paths[][PATH_SIZE], int *path_count);

#endif