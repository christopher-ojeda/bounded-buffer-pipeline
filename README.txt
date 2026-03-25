# Bounded Buffer Pipeline (Producer-Consumer)

## Description
This project implements a multithreaded processing pipeline in C using the producer-consumer model.  
It uses POSIX threads, mutexes, and semaphores to safely coordinate data flow between multiple threads.

## Features
- Bounded buffer implementation
- Producer-consumer synchronization
- POSIX semaphores (`sem_wait`, `sem_post`)
- Mutex-protected critical sections
- Circular buffer indexing
- Multi-stage pipeline (generator → processor → writer)
- Ordered output handling

## Tech Stack
- C
- POSIX Threads (pthread)
- POSIX Semaphores
- Linux

## Key Concepts
- Concurrency
- Thread synchronization
- Race condition prevention
- Deadlock avoidance
- Producer-consumer pattern

## How to Run
Compile on a Linux system:

```bash
gcc -pthread pipeline.c -o pipeline
./pipeline
```

## What I Learned
- How to coordinate multiple threads safely
- How semaphores control access to shared resources
- How to prevent race conditions and deadlocks
- How real-world concurrent pipelines are structured