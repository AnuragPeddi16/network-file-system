#include "log.h"

void init_log() {

    FILE* log_file = fopen("naming_server.log", "w");
    if (log_file) fclose(log_file);

}

// Logging mechanism
void log_message(const char* message) {

    printf("%s", message);

    FILE* log_file = fopen("naming_server.log", "a");
    if (log_file) {
        
        char s[1000];

        time_t t = time(NULL);
        struct tm * p = localtime(&t);

        strftime(s, sizeof s, "%d-%m-%Y %H:%M:%S", p);

        fprintf(log_file, "[%s] %s", s, message);
        fclose(log_file);
    }
}