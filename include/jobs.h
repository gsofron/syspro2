#ifndef JOBS_H
#define JOBS_H

#include "commands.h"
#include "utils.h"

typedef struct {
    char *id;
    char *full_command;
    int argc; // Number of arguments in full command
    Command command;
    int sock; // client's socket to send data back to
} Job;

// Creates and returns a job
Job *job_create(char *id, char *full_command, int argc, Command command, int sock);

// Destroys given job, freeing up all memory
void job_destroy(Job *job);

#endif