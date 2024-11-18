#ifndef _TRIES_H_
#define _TRIES_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

// Structure of a Trie node
typedef struct TrieNode {
    char *path_component;
    struct TrieNode *next_sibling;
    struct TrieNode *first_child;
} TrieNode;

void insert_path_trie(TrieNode *root, const char *path);
int search_path_trie(TrieNode *root, const char *path);
void delete_path_trie(TrieNode *root, const char *path);

#endif