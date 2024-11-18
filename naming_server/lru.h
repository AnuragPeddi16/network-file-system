#ifndef __LRU_h__
#define __LRU_h__

#define _POSIX_C_SOURCE 200112L

//basic header files
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <assert.h>
#include "common.h"

#define CACHE_SIZE 50

void cache_init();
StorageServer* cache_search_insert(char *path);
void cache_cleanup();

#endif