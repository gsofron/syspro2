#ifndef COMMANDS_H
#define COMMANDS_H

// Easily represent given command
typedef enum {
    NO_CMD = -1,
    EXIT,
    ISSUE_JOB,
    SET_CONCURRENCY,
    STOP,
    POLL
} Command;

#endif