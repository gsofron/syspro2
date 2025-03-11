#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "utils.h"

Queue *queue_create(void) {
    Queue *queue = calloc(1, sizeof(*queue));
    if (queue == NULL) perrorexit("calloc");
    return queue;
}

static QueueNode *queuenode_create(Job *job) {
    QueueNode *node = malloc(sizeof(*node));
    if (node == NULL) perrorexit("malloc");
    node->job = job;
    node->next = NULL;
    return node;
}

void queue_add(Queue *queue, Job *job) {
    QueueNode *newnode = queuenode_create(job);
    if (queue->size == 0) {
        queue->head = newnode;
        queue->tail = newnode;
    } else {
        queue->tail->next = newnode;
        queue->tail = newnode;
    }
    queue->size++;
}

Job *queue_remove(Queue *queue, char *jobid) {
    if (queue == NULL || queue->size == 0) return NULL;
    QueueNode *prev = NULL;
    QueueNode *node = queue->head;

    if (jobid != NULL) {
        while (true) {
            if (node == NULL) return NULL;
            else if (strcmp(node->job->id, jobid) == 0) break;
            prev = node;
            node = node->next;
        }
    }

    if (prev == NULL) queue->head = queue->head->next; // Remove head
    else prev->next = node->next;
    if (queue->tail == node) queue->tail = prev;

    Job *job = node->job;
    free(node);
    queue->size--;
    return job;
}

int queue_size(Queue *queue) {
    return queue->size;
}