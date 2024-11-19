#ifndef _MACROS_H_
#define _MACROS_H_

#define ASYNC_LIMIT 100

/* STATUS CODES */
#define OK 0
#define ACK 1
#define NOT_FOUND 2
#define FAILED 3
#define ASYNCHRONOUS_COMPLETE 4
#define SS_DOWN 5

/* COLORS */

#define COLOR_RESET   "\x1b[0m"

#define RED(x)     "\x1b[31m" x "\x1b[0m"
#define GREEN(x)   "\x1b[32m" x "\x1b[0m"
#define YELLOW(x)  "\x1b[33m" x "\x1b[0m"
#define BLUE(x)    "\x1b[34m" x "\x1b[0m"
#define MAGENTA(x) "\x1b[35m" x "\x1b[0m"
#define CYAN(x)    "\x1b[36m" x "\x1b[0m"

#endif