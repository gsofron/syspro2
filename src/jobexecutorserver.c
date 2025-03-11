#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "commands.h"
#include "queue.h"
#include "utils.h"

static struct {
    Queue *buf;                 // buf storing jobs waiting to be executed
    int capacity;               // buf's capacity (max size)

    int jobid_counter;          // jobID counter
    
    pthread_t *worker_threads;  // Worker threads
    int thread_pool_size;       // Num of worker threads
    int concurrency;            // concurrency level
    int active_workers;         // num of active workers (at most thread_pool_size)
    
    bool exit_program;          // Boolean var determining program status
} DATA;

static struct {
    pthread_mutex_t mtx_buf;
    pthread_mutex_t mtx_concurrency;
    pthread_mutex_t mtx_active_workers;
    pthread_mutex_t mtx_jobid;
} MUTEX;

static struct {
    pthread_cond_t wakeup_job;
    pthread_cond_t buf_not_full;
} CONDVAR;

/* Stores program's args into given variables (port, bufsize, thread_pool_size).
   Terminates program's execution if an args error is caught */
static void parse_args(int argc, char **argv, uint16_t *port, int *bufsize, int *thread_pool_size) {
    if (argc == 4 &&
        only_numeric_digits(argv[1]) && (*port = atoi(argv[1])) > 0 &&
        only_numeric_digits(argv[2]) && (*bufsize = atoi(argv[2])) > 0 &&
        only_numeric_digits(argv[3]) && (*thread_pool_size = atoi(argv[3])) > 0)
    {
        return; // Success
    }
    fprintf(stderr, "Usage: %s [portnum] [bufferSize] [threadPoolSize]\n", argv[0]);
    exit(EXIT_FAILURE);
}

// Implementation of controller threads
static void *thread_controller(void *arg) {
    int sock = *(int *)arg;
    free(arg);

    Command command;
    int bytes_to_read, len, argc, old_concurrency, new_concurrency;
    char *msg, *jobid;
    Job *job;
    char buf[1024]; // General purpose buffer

    fullread(sock, &command, sizeof(Command));
    switch (command) {
    // Format: command (int)
    case EXIT:
        DATA.exit_program = true;
        // Destroy buf and its contents
        pthread_mutex_lock(&MUTEX.mtx_buf);
        QueueNode *node = DATA.buf->head, *next;
        while (node != NULL) {
            next = node->next;
            if (write(node->job->sock, "SERVER TERMINATED BEFORE EXECUTION\n", 35) == -1) perrorexit("write");
            if (shutdown(node->job->sock, SHUT_WR) == -1) perrorexit("shutdown");
            job_destroy(node->job);
            free(node);
            node = next;
        }
        free(DATA.buf);
        pthread_mutex_unlock(&MUTEX.mtx_buf);
        // Wake up all suspended threads
        pthread_cond_broadcast(&CONDVAR.buf_not_full);
        pthread_cond_broadcast(&CONDVAR.wakeup_job);
        for (int i = 0; i < DATA.thread_pool_size; i++)
            if (pthread_join(DATA.worker_threads[i], NULL) != 0) errorexit("pthread_join");
        if (write(sock, "SERVER TERMINATED\n", 18) == -1) perrorexit("write");
        if (shutdown(sock, SHUT_WR) == -1) perrorexit("shutdown");
        // Free up memory
        free(DATA.worker_threads);
        if (pthread_mutex_destroy(&MUTEX.mtx_buf) != 0) errorexit("pthread_mutex_destroy");
        if (pthread_mutex_destroy(&MUTEX.mtx_jobid) != 0) errorexit("pthread_mutex_destroy");
        if (pthread_mutex_destroy(&MUTEX.mtx_active_workers) != 0) errorexit("pthread_mutex_destroy");
        if (pthread_mutex_destroy(&MUTEX.mtx_concurrency) != 0) errorexit("pthread_mutex_destroy");
        if (pthread_cond_destroy(&CONDVAR.wakeup_job) != 0) errorexit("pthread_cond_destroy");
        if (pthread_cond_destroy(&CONDVAR.buf_not_full) != 0) errorexit("pthread_cond_destroy");
        exit(EXIT_SUCCESS); // Terminate all threads
    case POLL:
        pthread_mutex_lock(&MUTEX.mtx_buf);
        for (QueueNode *node = DATA.buf->head; node != NULL; node = node->next) {
            if (write(sock, "<", 1) == -1) perrorexit("write");
            if (write(sock, node->job->id, strlen(node->job->id)) == -1) perrorexit("write");
            if (write(sock, ", ", 2) == -1) perrorexit("write");
            fullwrite(sock, node->job->full_command, strlen(node->job->full_command));
            if (write(sock, ">\n", 2) == -1) perrorexit("write");
        }
        pthread_mutex_unlock(&MUTEX.mtx_buf);
        if (shutdown(sock, SHUT_WR) == -1) perrorexit("shutdown");
        break;
    // Format: command (int) + new_concurrency (int)
    case SET_CONCURRENCY:
        pthread_mutex_lock(&MUTEX.mtx_concurrency);
        old_concurrency = DATA.concurrency;
        fullread(sock, &DATA.concurrency, sizeof(int));
        new_concurrency = DATA.concurrency;
        pthread_mutex_unlock(&MUTEX.mtx_concurrency);
        sprintf(buf, "CONCURRENCY SET AT %d\n", new_concurrency);
        if (write(sock, buf, strlen(buf)) == -1) perrorexit("write");
        if (shutdown(sock, SHUT_WR) == -1) perrorexit("shutdown");
        // Wake up relative number of workers
        for (int i = old_concurrency; i <= new_concurrency && i <= DATA.thread_pool_size; i++)
            pthread_cond_signal(&CONDVAR.wakeup_job);
        break;
    // Format: command (int) + len (int) + jobID (string)
    case STOP:
        fullread(sock, &len, sizeof(int));
        if ((jobid = malloc(len * sizeof(*jobid))) == NULL) perrorexit("malloc");
        fullread(sock, jobid, len);
        // (Try to) remove job with given jobID
        pthread_mutex_lock(&MUTEX.mtx_buf);
        job = queue_remove(DATA.buf, jobid);
        pthread_mutex_unlock(&MUTEX.mtx_buf);
        // Write response
        if (write(sock, "JOB ", 4) == -1) perrorexit("write");
        if (write(sock, jobid, len - 1) == -1) perrorexit("write");
        free(jobid);
        if (job != NULL) { // Job was present in buf
            if (write(sock, " REMOVED\n", 9) == -1) perrorexit("write");
            // Wakeup another job
            pthread_cond_signal(&CONDVAR.buf_not_full);
            // "Send" EOF to the commander that issued this job
            if (shutdown(job->sock, SHUT_WR) == -1) perrorexit("shutdown");
            // Destroy this job
            job_destroy(job);
        } else {
            if (write(sock, " NOTFOUND\n", 10) == -1) perrorexit("write");
        }
        if (shutdown(sock, SHUT_WR) == -1) perrorexit("shutdown");
        break;
    // Format: command (int) + num_of_args (int) + bytes_to_read (int) + command (string)
    case ISSUE_JOB:
        fullread(sock, &argc, sizeof(int));
        fullread(sock, &bytes_to_read, sizeof(int));
        if ((msg = malloc(bytes_to_read * sizeof(*msg))) == NULL) perrorexit("malloc");
        fullread(sock, msg, bytes_to_read);
        pthread_mutex_lock(&MUTEX.mtx_jobid);
        sprintf(buf, "job_%d", DATA.jobid_counter++);
        pthread_mutex_unlock(&MUTEX.mtx_jobid);
        job = job_create(buf, msg, argc, command, sock);
        free(msg);
        // Add the job to buf
        pthread_mutex_lock(&MUTEX.mtx_buf);
        while (queue_size(DATA.buf) == DATA.capacity) { // While buf is full
            pthread_cond_wait(&CONDVAR.buf_not_full, &MUTEX.mtx_buf);
            if (DATA.exit_program) {
                pthread_mutex_unlock(&MUTEX.mtx_buf);
                if (write(sock, "SERVER TERMINATED BEFORE EXECUTION\n", 35) == -1) perrorexit("write");
                if (shutdown(sock, SHUT_WR) == -1) perrorexit("shutdown");
                job_destroy(job);
                pthread_exit(NULL);
            }
        }
        queue_add(DATA.buf, job);
        pthread_mutex_unlock(&MUTEX.mtx_buf);
        // Write response back to jobCommander
        if (write(sock, "JOB <", 5) == -1) perrorexit("write");
        if (write(sock, job->id, strlen(job->id)) == -1) perrorexit("write");
        if (write(sock, ", ", 2) == -1) perrorexit("write");
        fullwrite(sock, job->full_command, strlen(job->full_command));
        if (write(sock, "> SUBMITTED\n", 12) == -1) perrorexit("write");
        // Perhaps a job should wakeup (cond is checked in worker-thread)
        pthread_cond_signal(&CONDVAR.wakeup_job);
        break;
    default:
        errorexit("Invalid command");
        break;
    }
    pthread_exit(NULL);
}

// Implementation of worker threads
static void *thread_worker(void *arg) {
    (void)arg;
    while (!DATA.exit_program) {
        pthread_mutex_lock(&MUTEX.mtx_active_workers);
        pthread_mutex_lock(&MUTEX.mtx_concurrency);
        pthread_mutex_lock(&MUTEX.mtx_buf);
        while (queue_size(DATA.buf) <= 0 || // Cases in which no more jobs should wake up
                DATA.active_workers >= DATA.concurrency ||
                DATA.active_workers >= DATA.thread_pool_size)
        {
            pthread_mutex_unlock(&MUTEX.mtx_buf);
            pthread_mutex_unlock(&MUTEX.mtx_concurrency);
            pthread_cond_wait(&CONDVAR.wakeup_job, &MUTEX.mtx_active_workers);
            if (DATA.exit_program) {
                pthread_mutex_unlock(&MUTEX.mtx_active_workers);
                pthread_exit(NULL);
            }
            pthread_mutex_lock(&MUTEX.mtx_concurrency);
            pthread_mutex_lock(&MUTEX.mtx_buf);
        }
        Job *job = queue_remove(DATA.buf, NULL);
        pthread_mutex_unlock(&MUTEX.mtx_buf);
        pthread_mutex_unlock(&MUTEX.mtx_concurrency);
        DATA.active_workers++;
        pthread_mutex_unlock(&MUTEX.mtx_active_workers);

        pthread_cond_signal(&CONDVAR.buf_not_full); // Just removed a job from buf

        // Tokenize command in order to execute it
        char **argv = malloc((job->argc + 1) * sizeof(*argv));
        if (argv == NULL) perrorexit("malloc");
        char *cmd_copy = duplicate_str(job->full_command);
        char *the_rest = cmd_copy;
        char *token;
        int i = 0;
        while ((token = strtok_r(the_rest, " ", &the_rest)) != NULL) // strtok_r is thread-safe
            argv[i++] = token;
        argv[i] = NULL;

        char buf[256];
        int fd;
        pid_t pid;
        struct stat st;
        char *resp;
        switch (pid = fork()) {
        case -1:
            perrorexit("fork");
            break;
        case 0:
            // Create the PID.output file
            sprintf(buf, "%d.output", getpid());
            if ((fd = open(buf, O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) perrorexit("open");
            if (dup2(fd, STDOUT_FILENO) == -1) perrorexit("dup2");
            if (close(fd) == -1) perrorexit("close");
            // Execute job
            execvp(argv[0], argv);
            perrorexit("execvp");
            break;
        default:
            if (waitpid(pid, NULL, 0) != pid) perrorexit("waitpid");
            // Read program's output
            sprintf(buf, "%d.output", pid);
            if ((fd = open(buf, O_RDONLY)) == -1) perrorexit("open");
            if (fstat(fd, &st) == -1) perrorexit("fstat");
            if ((resp = malloc(st.st_size * sizeof(*resp))) == NULL) perrorexit("malloc");
            fullread(fd, resp, st.st_size);
            if (close(fd) == -1) perrorexit("read");
            // Write response
            sprintf(buf, "----- %s output start ------\n\n", job->id);
            if (write(job->sock, buf, strlen(buf)) == -1) perrorexit("write");
            fullwrite(job->sock, resp, st.st_size);
            sprintf(buf, "\n------ %s output end -------\n", job->id);
            if (write(job->sock, buf, strlen(buf)) == -1) perrorexit("write");
            if (shutdown(job->sock, SHUT_WR) == -1) perrorexit("shutdown");
            free(resp);
            break;
        }
        free(cmd_copy);
        free(argv);
        sprintf(buf, "%d.output", pid);
        if (unlink(buf) == -1) perrorexit("unlink");
        job_destroy(job);
        pthread_mutex_lock(&MUTEX.mtx_active_workers);
        DATA.active_workers--;
        pthread_mutex_unlock(&MUTEX.mtx_active_workers);
    }
    pthread_exit(NULL);
}

int main(int argc, char **argv) {
    // Assure that program arguments are valid
    uint16_t port;
    parse_args(argc, argv, &port, &DATA.capacity, &DATA.thread_pool_size);

    DATA.buf = queue_create();
    DATA.jobid_counter = 1;
    DATA.concurrency = 1;
    DATA.active_workers = 0;
    DATA.exit_program = false;
    // Init mutexes
    if (pthread_mutex_init(&MUTEX.mtx_buf, NULL) != 0) errorexit("pthread_mutex_init");
    if (pthread_mutex_init(&MUTEX.mtx_active_workers, NULL) != 0) errorexit("pthread_mutex_init");
    if (pthread_mutex_init(&MUTEX.mtx_concurrency, NULL) != 0) errorexit("pthread_mutex_init");
    if (pthread_mutex_init(&MUTEX.mtx_jobid, NULL) != 0) errorexit("pthread_mutex_init");
    // Init conditional variables
    if (pthread_cond_init(&CONDVAR.wakeup_job, NULL) != 0) errorexit("pthread_cond_init");
    if (pthread_cond_init(&CONDVAR.buf_not_full, NULL) != 0) errorexit("pthread_cond_init");
    // Initiate worker threads
    if ((DATA.worker_threads = malloc(DATA.thread_pool_size * sizeof(*DATA.worker_threads))) == NULL)
        perrorexit("malloc");
    for (int i = 0; i < DATA.thread_pool_size; i++)
        if (pthread_create(&DATA.worker_threads[i], NULL, thread_worker, NULL) != 0)
            errorexit("pthread_create");

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) perrorexit("socket");
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl(INADDR_ANY);
    server.sin_port = htons(port);
    if (bind(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1) perrorexit("bind");
    if (listen(sockfd, 12) == -1) perrorexit("listen");
    printf("Listening for connections to port %d\n", port);
    pthread_t p;
    int *newsock, tmp;
    while (!DATA.exit_program) {
        if ((tmp = accept(sockfd, NULL, NULL)) == -1) perrorexit("accept");
        printf("Accepted connection\n");
        if ((newsock = malloc(sizeof(*newsock))) == NULL) perrorexit("malloc");
        *newsock = tmp;
        if (pthread_create(&p, NULL, thread_controller, newsock) != 0) errorexit("pthread_create");
        if (pthread_detach(p) != 0) errorexit("pthread_detach");
    }
    pthread_exit(NULL);
}