#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Crime types enum
typedef enum {
    BANK_ROBBERY,
    JEWELRY_ROBBERY,
    DRUG_TRAFFICKING,
    ARTWORK_ROBBERY,
    KIDNAPPING,
    BLACKMAILING,
    ARM_TRAFFICKING,
    NUM_CRIME_TYPES
} CrimeType;

// Configuration structure to hold all user-defined parameters
typedef struct {
    // Gang configuration
    int min_gangs;
    int max_gangs;
    int min_members_per_gang;
    int max_members_per_gang;
    int gang_ranks;
    
    // Crime planning
    int preparation_time_min;
    int preparation_time_max;
    int min_preparation_level;
    int max_preparation_level;
    int false_info_probability;
    
    // Secret agents
    int agent_infiltration_success_rate;
    int agent_suspicion_threshold;
    int police_action_threshold;
    int truth_gain;        // Knowledge gain when receiving truthful information
    int false_penalty;     // Knowledge penalty when receiving false information
    
    // Mission outcomes
    int mission_success_rate_base;
    int member_death_probability;
    int prison_time_min;
    int prison_time_max;
    
    // Termination conditions
    int max_thwarted_plans;
    int max_successful_plans;
    int max_executed_agents;
    
    // Visualization
    int visualization_refresh_rate;
} SimulationConfig;

// Function prototypes
SimulationConfig load_config(const char* config_file);
void print_config(SimulationConfig config);

#endif /* CONFIG_H */
