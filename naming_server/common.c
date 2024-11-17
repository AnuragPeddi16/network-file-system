#include "common.h"

void print_error(char* message) {

    char* str = (char*) malloc(sizeof(char)*(strlen(message)+50));
    sprintf(str, RED("%s"), message);
    perror(str);

    free(str);

}

char* str_before_last_slash(char* str) {

    char* substring = NULL;
    char* last = strrchr(str, '/');

    if (last != NULL) {

        int length = last - str;
        char *substring = malloc(length+1);
        strncpy(substring, str, length);
        substring[length] = '\0';

    }

    return substring;

}