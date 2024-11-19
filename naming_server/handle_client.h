#ifndef _HANDLE_H_
#define _HANDLE_H_

#include "common.h"
#include "lru.h"

void *client_handler(void *client_sock_fd);
int add_backups(StorageServer* ss);

#endif