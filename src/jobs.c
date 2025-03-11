#include <stdlib.h>

#include "jobs.h"

Job *job_create(char *id, char *full_command, int argc, Command command, int sock) {
    Job *job = malloc(sizeof(*job));
    if (job == NULL) perrorexit("malloc");
    job->id = duplicate_str(id);
    job->full_command = duplicate_str(full_command);
    job->argc = argc;
    job->command = command;
    job->sock = sock;
    return job;
}

void job_destroy(Job *job) {
    if (job == NULL) return;
    if (job->id != NULL) free(job->id);
    if (job->full_command != NULL) free(job->full_command);
    free(job);
}