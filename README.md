# Multi-Threaded Client-Server Network System

The goal of this project was to implement a multi-threaded network application in C that executes and manages jobs submitted remotely over a TCP-based client-server architecture. The system consists of a job execution server that schedules and executes tasks using worker threads and a client application for remote job submission, concurrency control, and job management. The implementation ensures efficiency, correctness, and adaptability under high-load conditions.


## Components

- `jobExecutorServer`: A multi-threaded server that manages incoming job requests, schedules execution, and maintains concurrency control.
- `jobCommander`: A client application that allows users to submit jobs, adjust concurrency, cancel tasks, and terminate the server.


## Key Features

- *Multi-threaded Server*: Uses worker threads for parallel job execution, synchronized via POSIX condition variables and mutexes.
- *Network Communication*: Implements a TCP-based protocol for efficient client-server interaction.
- *Dynamic Concurrency Control*: Allows real-time adjustment of active worker threads while ensuring correctness.
- *Job Queueing & Management*: Supports job submission, cancellation, and polling of pending jobs.
- *Graceful Shutdown*: Ensures safe termination while completing active tasks.


## Usage


### Compilation  
```bash
make
```

### Running the Server

```
./bin/jobExecutorServer [portnum] [bufferSize] [threadPoolSize]
```

Example:

```
./bin/jobExecutorServer 7856 8 5
```

This starts the server on port 7856 with a job queue buffer size of 8 and a thread pool of 5 worker threads.

### Running the Client

```
./bin/jobCommander [serverName] [portNum] [command]
```

Example (*issueJob* command):

```
./bin/jobCommander localhost 7856 issueJob ls -l
```

This connects to a server running on localhost at port 7856 and submits a job to list directory contents.

### Available Commands (Client Program)

|Command|Description|Example|
|----------|----------|----------|
|`issueJob <job>` | Submits a job (a shell command) for execution. | `issueJob ls -l`|
|`setConcurrency <N>` | Sets the number of worker threads actively executing jobs. | `setConcurrency 4`|
|`stop <jobID>` | Removes a job from the queue (if not yet running). | `stop job_2`|
|`poll` | Lists all queued jobs waiting for execution. | `poll` |
|`exit` | Gracefully shuts down the server after completing running jobs. | `exit` |


## University Project

This project was developed as part of the 2nd assignment of the course **"Systems Programming"** (hence the name *syspro2*) course (6th semester, Spring 2024, Professor Alexandros Ntoulas) at the National and Kapodistrian University of Athens (NKUA). It received a grade of 100/100 along with excellent feedback.
