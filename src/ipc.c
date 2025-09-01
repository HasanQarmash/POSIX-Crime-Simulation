#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <errno.h>
#include "../include/ipc.h"
#include "../include/utils.h"

// Define keys for IPC resources
#define REPORT_QUEUE_KEY 0x1234
#define SHARED_MEMORY_KEY 0x5678
#define SEMAPHORE_KEY 0x9ABC

// Size of shared memory segment
#define SHARED_MEMORY_SIZE sizeof(SharedState)

// Number of semaphores in the set
#define NUM_SEMAPHORES 1

// Create message queue for intelligence reports
int create_report_queue() {
    int queue_id = msgget(REPORT_QUEUE_KEY, IPC_CREAT | 0666);
    
    if (queue_id == -1) {
        perror("Failed to create message queue");
        exit(1);
    }
    
    log_message("Created message queue with ID %d", queue_id);
    return queue_id;
}

// Destroy message queue
void destroy_report_queue(int queue_id) {
    if (msgctl(queue_id, IPC_RMID, NULL) == -1) {
        perror("Failed to destroy message queue");
    }
    else {
        log_message("Destroyed message queue with ID %d", queue_id);
    }
}

// Send an intelligence report
int send_report(int queue_id, IntelligenceReport report) {
    ReportMessage msg;
    msg.mtype = 1;  // Message type (can be used for filtering)
    msg.report = report;
    
    int result = msgsnd(queue_id, &msg, sizeof(IntelligenceReport), 0);
    
    if (result == -1) {
        perror("Failed to send report");
    }
    
    return result;
}

// Receive an intelligence report
int receive_report(int queue_id, IntelligenceReport* report) {
    ReportMessage msg;
    
    int result = msgrcv(queue_id, &msg, sizeof(IntelligenceReport), 0, IPC_NOWAIT);
    
    if (result == -1) {
        if (errno != ENOMSG) {
            perror("Failed to receive report");
        }
    }
    else {
        *report = msg.report;
    }
    
    return result;
}

// Create shared memory segment
int create_shared_memory() {
    int shm_id = shmget(SHARED_MEMORY_KEY, SHARED_MEMORY_SIZE, IPC_CREAT | 0666);
    
    if (shm_id == -1) {
        perror("Failed to create shared memory");
        exit(1);
    }
    
    log_message("Created shared memory segment with ID %d", shm_id);
    return shm_id;
}

// Destroy shared memory segment
void destroy_shared_memory(int shm_id) {
    if (shmctl(shm_id, IPC_RMID, NULL) == -1) {
        perror("Failed to destroy shared memory");
    }
    else {
        log_message("Destroyed shared memory segment with ID %d", shm_id);
    }
}

// Attach to shared memory
SharedState* attach_shared_memory(int shm_id) {
    SharedState* shm_ptr = (SharedState*)shmat(shm_id, NULL, 0);
    
    if (shm_ptr == (SharedState*)-1) {
        perror("Failed to attach to shared memory");
        exit(1);
    }
    
    return shm_ptr;
}

// Detach from shared memory
void detach_shared_memory(SharedState* shm_ptr) {
    if (shmdt(shm_ptr) == -1) {
        perror("Failed to detach from shared memory");
    }
}

// Create semaphore set
int create_semaphore_set() {
    int sem_id = semget(SEMAPHORE_KEY, NUM_SEMAPHORES, IPC_CREAT | 0666);
    
    if (sem_id == -1) {
        perror("Failed to create semaphore set");
        exit(1);
    }
    
    // Initialize semaphores
    union semun {
        int val;
        struct semid_ds* buf;
        unsigned short* array;
    } arg;
    
    arg.val = 1;  // Binary semaphore
    
    if (semctl(sem_id, 0, SETVAL, arg) == -1) {
        perror("Failed to initialize semaphore");
        exit(1);
    }
    
    log_message("Created semaphore set with ID %d", sem_id);
    return sem_id;
}

// Destroy semaphore set
void destroy_semaphore_set(int sem_id) {
    if (semctl(sem_id, 0, IPC_RMID) == -1) {
        perror("Failed to destroy semaphore set");
    }
    else {
        log_message("Destroyed semaphore set with ID %d", sem_id);
    }
}

// Wait on semaphore (P operation)
void semaphore_wait(int sem_id, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    
    if (semop(sem_id, &sb, 1) == -1) {
        perror("Semaphore wait operation failed");
    }
}

// Signal semaphore (V operation)
void semaphore_signal(int sem_id, int sem_num) {
    struct sembuf sb;
    sb.sem_num = sem_num;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    
    if (semop(sem_id, &sb, 1) == -1) {
        perror("Semaphore signal operation failed");
    }
}
