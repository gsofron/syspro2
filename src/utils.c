#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "utils.h"

void perrorexit(char *s) {
    perror(s);
    exit(EXIT_FAILURE);
}

void errorexit(char *s) {
    fprintf(stderr, "%s\n", s);
    exit(EXIT_FAILURE);
}

char *duplicate_str(char *str) {
    char *newstr = malloc((strlen(str) + 1) * sizeof(*newstr));
    if (newstr == NULL) perrorexit("malloc");
    strcpy(newstr, str);
    return newstr;
}

bool only_numeric_digits(char *str) {
    do {
        if (!(*str >= '0' && *str <= '9')) return false;
        str++;
    } while (*str != '\0');
    return true;
}

void fullread(int fd, void *buf, size_t count) {
    ssize_t cbr; // Current number of bytes read
    size_t tbr = 0; // Total number of bytes read
    while (tbr < count) {
        if ((cbr = read(fd, (char *)buf + tbr, count - tbr)) == -1) {
            if (errno == EINTR) continue;
            perrorexit("read");
        }
        tbr += cbr;
    }
}

void fullwrite(int fd, void *buf, size_t count) {
    ssize_t cbw; // Current number of bytes written
    size_t tbw = 0; // Total number of bytes written
    while (tbw < count) {
        if ((cbw = write(fd, (char *)buf + tbw, count - tbw)) == -1) {
            if (errno == EINTR) continue;
            perrorexit("write");
        }
        tbw += cbw;
    }
}