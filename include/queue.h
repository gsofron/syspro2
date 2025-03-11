#ifndef QUEUE_H
#define QUEUE_H

#include "jobs.h"

typedef struct queuenode QueueNode;
struct queuenode {
    Job *job;
    QueueNode *next;
};

typedef struct {
    int size;
    QueueNode *head;
    QueueNode *tail;
} Queue;

// Creates and returns an empty queue
Queue *queue_create(void);

// Adds given job at the end of the queue
void queue_add(Queue *queue, Job *job);

// Removes and returns job with given ID. If NULL is given, it removes queue's head (FIFO).
// In case of error, NULL is returned
Job *queue_remove(Queue *queue, char *jobid);

// Returns the number of elements that given queue contains
int queue_size(Queue *queue);

#endif