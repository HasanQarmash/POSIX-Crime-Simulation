#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "../include/config.h"

// Function to trim whitespace from a string
static char* trim(char* str) {
    char* end;
    
    // Trim leading space
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == 0) // All spaces?
        return str;
    
    // Trim trailing space
    end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    
    // Write new null terminator
    *(end + 1) = 0;
    
    return str;
}

// Load configuration from file
SimulationConfig load_config(const char* config_file) {
    SimulationConfig config;
    FILE* file = fopen(config_file, "r");
    
    if (file == NULL) {
        fprintf(stderr, "Error: Unable to open config file %s\n", config_file);
        exit(1);
    }
    
    // Set default values
    config.min_gangs = 3;
    config.max_gangs = 5;
    config.min_members_per_gang = 5;
    config.max_members_per_gang = 10;
    config.gang_ranks = 5;
    config.preparation_time_min = 5;
    config.preparation_time_max = 20;
    config.min_preparation_level = 70;
    config.max_preparation_level = 100;
    config.false_info_probability = 30;
    config.agent_infiltration_success_rate = 60;
    config.agent_suspicion_threshold = 75;
    config.police_action_threshold = 80;
    config.truth_gain = 10;        // Default knowledge gain
    config.false_penalty = 5;      // Default knowledge penalty
    config.mission_success_rate_base = 50;
    config.member_death_probability = 10;
    config.prison_time_min = 5;
    config.prison_time_max = 15;
    config.max_thwarted_plans = 10;
    config.max_successful_plans = 15;
    config.max_executed_agents = 5;
    config.visualization_refresh_rate = 1000;
    
    // Parse configuration file
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        // Skip comments and empty lines
        if (line[0] == '#' || line[0] == '\n') {
            continue;
        }
        
        // Remove newline character
        line[strcspn(line, "\n")] = 0;
        
        // Split line into key and value
        char* sep = strchr(line, '=');
        if (sep == NULL) {
            continue;
        }
        
        *sep = 0;
        char* key = trim(line);
        char* value = trim(sep + 1);
        
        // Parse configuration parameters
        if (strcmp(key, "MIN_GANGS") == 0) {
            config.min_gangs = atoi(value);
        }
        else if (strcmp(key, "MAX_GANGS") == 0) {
            config.max_gangs = atoi(value);
        }
        else if (strcmp(key, "MIN_MEMBERS_PER_GANG") == 0) {
            config.min_members_per_gang = atoi(value);
        }
        else if (strcmp(key, "MAX_MEMBERS_PER_GANG") == 0) {
            config.max_members_per_gang = atoi(value);
        }
        else if (strcmp(key, "GANG_RANKS") == 0) {
            config.gang_ranks = atoi(value);
        }
        else if (strcmp(key, "PREPARATION_TIME_MIN") == 0) {
            config.preparation_time_min = atoi(value);
        }
        else if (strcmp(key, "PREPARATION_TIME_MAX") == 0) {
            config.preparation_time_max = atoi(value);
        }
        else if (strcmp(key, "MIN_PREPARATION_LEVEL") == 0) {
            config.min_preparation_level = atoi(value);
        }
        else if (strcmp(key, "MAX_PREPARATION_LEVEL") == 0) {
            config.max_preparation_level = atoi(value);
        }
        else if (strcmp(key, "FALSE_INFO_PROBABILITY") == 0) {
            config.false_info_probability = atoi(value);
        }
        else if (strcmp(key, "AGENT_INFILTRATION_SUCCESS_RATE") == 0) {
            config.agent_infiltration_success_rate = atoi(value);
        }
        else if (strcmp(key, "AGENT_SUSPICION_THRESHOLD") == 0) {
            config.agent_suspicion_threshold = atoi(value);
        }
        else if (strcmp(key, "POLICE_ACTION_THRESHOLD") == 0) {
            config.police_action_threshold = atoi(value);
        }
        else if (strcmp(key, "TRUTH_GAIN") == 0) {
            config.truth_gain = atoi(value);
        }
        else if (strcmp(key, "FALSE_PENALTY") == 0) {
            config.false_penalty = atoi(value);
        }
        else if (strcmp(key, "MISSION_SUCCESS_RATE_BASE") == 0) {
            config.mission_success_rate_base = atoi(value);
        }
        else if (strcmp(key, "MEMBER_DEATH_PROBABILITY") == 0) {
            config.member_death_probability = atoi(value);
        }
        else if (strcmp(key, "PRISON_TIME_MIN") == 0) {
            config.prison_time_min = atoi(value);
        }
        else if (strcmp(key, "PRISON_TIME_MAX") == 0) {
            config.prison_time_max = atoi(value);
        }
        else if (strcmp(key, "MAX_THWARTED_PLANS") == 0) {
            config.max_thwarted_plans = atoi(value);
        }
        else if (strcmp(key, "MAX_SUCCESSFUL_PLANS") == 0) {
            config.max_successful_plans = atoi(value);
        }
        else if (strcmp(key, "MAX_EXECUTED_AGENTS") == 0) {
            config.max_executed_agents = atoi(value);
        }
        else if (strcmp(key, "VISUALIZATION_REFRESH_RATE") == 0) {
            config.visualization_refresh_rate = atoi(value);
        }
    }
    
    fclose(file);
    return config;
}

// Print configuration values
void print_config(SimulationConfig config) {
    printf("=== Simulation Configuration ===\n");
    printf("Gang Configuration:\n");
    printf("  - Number of gangs: %d-%d\n", config.min_gangs, config.max_gangs);
    printf("  - Members per gang: %d-%d\n", config.min_members_per_gang, config.max_members_per_gang);
    printf("  - Number of ranks: %d\n", config.gang_ranks);
    
    printf("\nCrime Planning:\n");
    printf("  - Preparation time: %d-%d time units\n", config.preparation_time_min, config.preparation_time_max);
    printf("  - Required preparation level: %d-%d%%\n", config.min_preparation_level, config.max_preparation_level);
    printf("  - False information probability: %d%%\n", config.false_info_probability);
    
    printf("\nSecret Agents:\n");
    printf("  - Infiltration success rate: %d%%\n", config.agent_infiltration_success_rate);
    printf("  - Suspicion threshold: %d%%\n", config.agent_suspicion_threshold);
    printf("  - Police action threshold: %d%%\n", config.police_action_threshold);
    printf("  - Truth gain: %d\n", config.truth_gain);
    printf("  - False penalty: %d\n", config.false_penalty);
    
    printf("\nMission Outcomes:\n");
    printf("  - Base mission success rate: %d%%\n", config.mission_success_rate_base);
    printf("  - Member death probability: %d%%\n", config.member_death_probability);
    printf("  - Prison time: %d-%d time units\n", config.prison_time_min, config.prison_time_max);
    
    printf("\nTermination Conditions:\n");
    printf("  - Max thwarted plans: %d\n", config.max_thwarted_plans);
    printf("  - Max successful plans: %d\n", config.max_successful_plans);
    printf("  - Max executed agents: %d\n", config.max_executed_agents);
    
    printf("\nVisualization:\n");
    printf("  - Refresh rate: %d ms\n", config.visualization_refresh_rate);
    printf("==============================\n\n");
}
