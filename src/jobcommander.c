#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "commands.h"
#include "utils.h"

// Returns provided command or NO_CMD if any type of args-error has occured
static Command parse_args(int argc, char **argv) {
    if (argc <= 3) return NO_CMD;
    if (!only_numeric_digits(argv[2])) return NO_CMD;
    
    Command command = NO_CMD;
    switch (argc) {
    case 4:
        if (strcmp(argv[3], "exit") == 0)
            command = EXIT;
        else if (strcmp(argv[3], "poll") == 0)
            command = POLL;
        break;
    case 5:
        if (strcmp(argv[3], "issueJob") == 0)
            command = ISSUE_JOB;
        else if (strcmp(argv[3], "setConcurrency") == 0 && only_numeric_digits(argv[4]))
            command = SET_CONCURRENCY;
        else if (strcmp(argv[3], "stop") == 0)
            command = STOP;
        break;
    default:
        if (strcmp(argv[3], "issueJob") == 0)
            command = ISSUE_JOB;
        break;
    }
    return command;
}

// Constructs and returns the message that will be sent to the server
static char *msg_to_server(Command command, int ac, char **args, int *msglen) {
    char *msg;
    int new_concurrency, len, bytes_to_read, total_bytes;
    switch (command) {
    // Format: command (int)
    case EXIT:
    case POLL:
        msg = malloc(sizeof(Command) * sizeof(*msg));
        if (msg == NULL) perrorexit("malloc");
        memcpy(msg, &command, sizeof(Command));
        *msglen = sizeof(Command);
        break;
    // Format: command (int) + new_concurrency (int)
    case SET_CONCURRENCY:
        if ((new_concurrency = atoi(args[0])) == 0) errorexit("Concurrency must be a positive number");
        msg = malloc((sizeof(Command) + sizeof(int)) * sizeof(*msg));
        if (msg == NULL) perrorexit("malloc");
        memcpy(msg, &command, sizeof(Command));
        memcpy(msg + sizeof(Command), &new_concurrency, sizeof(int));
        *msglen = sizeof(Command) + sizeof(int);
        break;
    // Format: command (int) + len (int) + jobID (string)
    case STOP:
        len = strlen(args[0]) + 1;
        msg = malloc((sizeof(Command) + sizeof(int) + len) * sizeof(*msg));
        if (msg == NULL) perrorexit("malloc");
        memcpy(msg, &command, sizeof(Command));
        memcpy(msg + sizeof(Command), &len, sizeof(int));
        memcpy(msg + sizeof(Command) + sizeof(int), args[0], len);
        *msglen = sizeof(Command) + sizeof(int) + len;
        break;
    // Format: command (int) + num_of_args (int) + bytes_to_read (int) + command (string)
    case ISSUE_JOB:
        bytes_to_read = 1; // For the '\0' at the end
        for (int i = 0; i < ac; i++)
            bytes_to_read += strlen(args[i]);
        // Add nessecary spaces between the command's arguments
        bytes_to_read = bytes_to_read + ac - 1;
        total_bytes = sizeof(Command) + 2 * sizeof(int) + bytes_to_read;
        msg = malloc(total_bytes * sizeof(*msg));
        if (msg == NULL) perrorexit("malloc");
        memcpy(msg, &command, sizeof(Command));
        memcpy(msg + sizeof(Command), &ac, sizeof(int));
        memcpy(msg + sizeof(Command) + sizeof(int), &bytes_to_read, sizeof(int));
        memset(msg + sizeof(Command) + 2 * sizeof(int), '\0', 1);
        for (int i = 0; i < ac; i++) {
            if (i >= 1) strcat(msg + sizeof(Command) + 2 * sizeof(int), " ");
            strcat(msg + sizeof(Command) + 2 * sizeof(int), args[i]);
        }
        *msglen = total_bytes;
        break;
    default:
        errorexit("Invalid command");
        break;
    }
    return msg;
}

int main(int argc, char **argv) {
    // Assure that program arguments are valid and store the given command
    Command command = parse_args(argc, argv);
    if (command == NO_CMD) {
        fprintf(stderr, "Usage: %s [serverName] [portNum] [jobCommanderInputCommand]\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    char *server_name = argv[1];
    uint16_t port = atoi(argv[2]);

    // Init socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) perrorexit("socket");
    struct hostent *rem = gethostbyname(server_name);
    if (rem == NULL) errorexit("gethostbyname");
    struct sockaddr_in server;
    server.sin_family = AF_INET;
    memcpy(&(server.sin_addr), rem->h_addr, rem->h_length);
    server.sin_port = htons(port);
    // Initiate connection
    if (connect(sockfd, (struct sockaddr *)&server, sizeof(server)) == -1) perrorexit("connect");
    printf("Connecting to %s port %d\n", server_name, port);
    
    int msglen;
    char *msg = msg_to_server(command, argc - 4, argv + 4, &msglen);
    // Write message to server
    fullwrite(sockfd, msg, msglen);
    if (shutdown(sockfd, SHUT_WR) == -1) perrorexit("shutdown");
    free(msg);

    char buf[1024]; // General purpose buffer
    int n;
    // Read server's response
    while ((n = read(sockfd, buf, sizeof(buf) - 1)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    if (n == -1) perrorexit("read");

    if (close(sockfd) == -1) perrorexit("close");
    
    exit(EXIT_SUCCESS);
}