#ifndef GANG_H
#define GANG_H

#include <pthread.h>
#include <stdbool.h>
#include "config.h"

// Gang member structure
typedef struct {
    int id;
    int rank;
    int preparation_level;
    int knowledge;    // Knowledge about current mission
    int suspicion;    // How suspicious the member appears
    bool is_secret_agent;
    bool alive;       // Whether the member is alive
    bool in_prison;   // Whether the member is in prison
    int knowledge_rate;
    pthread_t thread;
    void* gang_ptr;  // Pointer back to the gang
} GangMember;

// Gang structure
typedef struct {
    int id;
    int num_members;
    int num_ranks;
    GangMember* members;
    
    // Gang state
    CrimeType current_target;
    int preparation_time;
    int required_preparation_level;
    bool is_active;
    bool is_in_prison;
    int prison_time_remaining;
    int false_info_probability;
    int truth_gain;        // Knowledge gain when receiving truthful information
    int false_penalty;     // Knowledge penalty when receiving false information
    
    // Statistics
    int successful_missions;
    int thwarted_missions;
    int executed_agents;
    
    // Synchronization
    pthread_mutex_t gang_mutex;
    pthread_cond_t gang_cond;
    
    // IPC
    int report_queue_id;
    
    // Process ID
    pid_t pid;
} Gang;

// Function prototypes
void initialize_gang(Gang* gang, int id, int num_members, int num_ranks, SimulationConfig config);
void* gang_member_routine(void* arg);
void* gang_leader_routine(void* arg);
void plan_new_mission(Gang* gang, SimulationConfig config);
void execute_mission(Gang* gang, SimulationConfig config);
void investigate_for_agents(Gang* gang, SimulationConfig config);
void cleanup_gang(Gang* gang);

// Helper function to determine if truth or disinformation is delivered based on rank difference
bool deliver_truth(int sender_rank, int receiver_rank, int false_info_probability);

#endif /* GANG_H */
