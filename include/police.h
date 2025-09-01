#ifndef POLICE_H
#define POLICE_H

#include <pthread.h>
#include <stdbool.h>
#include <sys/types.h>
#include "config.h"
#include "gang.h"

// Information structure passed from agents to police
typedef struct {
    int gang_id;
    int agent_id;
    CrimeType suspected_target;
    int suspicion_level;
    bool is_reliable;
} IntelligenceReport;

// Police structure
typedef struct {
    // Intelligence reports
    IntelligenceReport* reports;
    int report_capacity;
    int num_reports;
    
    // Statistics
    int thwarted_missions;
    int total_agents;
    int lost_agents;
    
    // Synchronization
    pthread_mutex_t police_mutex;
    pthread_cond_t police_cond;
    
    // IPC mechanism for reports from agents
    int report_queue_id;  // Message queue ID
} Police;

// Function prototypes
void initialize_police(Police* police, SimulationConfig config);
void process_intelligence(Police* police, IntelligenceReport report, SimulationConfig config);
bool decide_on_action(Police* police, int gang_id, SimulationConfig config);
void arrest_gang_members(Police* police, int gang_id, SimulationConfig config);
void submit_report(IntelligenceReport report, int queue_id);
void* police_routine(void* arg);
void cleanup_police(Police* police);

#endif /* POLICE_H */
