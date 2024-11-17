#include "common.h"

void print_error(char* message) {

    char* str = (char*) malloc(sizeof(char)*(strlen(message)+50));
    sprintf(str, RED("%s"), message);
    perror(str);

    free(str);

}