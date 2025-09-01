#ifndef IPC_H
#define IPC_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include "police.h"

// Define keys for IPC resources
#define REPORT_QUEUE_KEY 0x1234
#define SHARED_MEMORY_KEY 0x5678
#define SEMAPHORE_KEY 0x9ABC

// Message queue structure for intelligence reports
typedef struct {
    long mtype;  // Message type
    IntelligenceReport report;
} ReportMessage;

// Shared memory structure for simulation state
typedef struct {
    int num_gangs;
    int total_successful_missions;
    int total_thwarted_missions;
    int total_executed_agents;
    bool simulation_running;
    
    // Gang arrest status - used for police to communicate with gangs
    struct {
        bool is_arrested;
        int prison_time;
        bool arrest_notification_seen;
    } gang_status[100]; // Assuming max 100 gangs
} SharedState;

// Function prototypes
int create_report_queue();
void destroy_report_queue(int queue_id);
int send_report(int queue_id, IntelligenceReport report);
int receive_report(int queue_id, IntelligenceReport* report);

int create_shared_memory();
void destroy_shared_memory(int shm_id);
SharedState* attach_shared_memory(int shm_id);
void detach_shared_memory(SharedState* shm_ptr);

int create_semaphore_set();
void destroy_semaphore_set(int sem_id);
void semaphore_wait(int sem_id, int sem_num);
void semaphore_signal(int sem_id, int sem_num);

#endif /* IPC_H */
