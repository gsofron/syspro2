#ifndef UTILS_H
#define UTILS_H

#include <stdbool.h>
#include <sys/types.h>

// Prints given message using perror() and calls exit() with code EXIT_FAILURE
void perrorexit(char *s);

// Prints given error message to stderr and calls exit() with code EXIT_FAILURE
void errorexit(char *s);

// Returns a dynamically allocated copy of given string
char *duplicate_str(char *str);

// Returns true only if given string is solely composed of digits from 0-9
bool only_numeric_digits(char *str);

// A modified version of the read() syscall which reads all desired bytes
void fullread(int fd, void *buf, size_t count);

/* A modified version of the write() syscall which writes all desired bytes.
   This version is only useful when is is needed to write very large number of
   data and it is ok for the writing to happen in multiple write() calls */
void fullwrite(int fd, void *buf, size_t count);

#endif