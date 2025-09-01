#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/msg.h>
#include <errno.h>
#include <pthread.h>
#include "../include/config.h"
#include "../include/gang.h"
#include "../include/police.h"
#include "../include/ipc.h"
#include "../include/utils.h"
#include "../include/visualization.h"

// Global variables
SimulationConfig config;
VisualizationContext viz_context;
SharedState* shared_state = NULL;
int shm_id = -1;
int sem_id = -1;
int report_queue_id = -1;
pid_t* gang_pids = NULL;
pid_t police_pid = -1;

// Function to handle cleanup on exit
void cleanup() {
    // Clean up IPC resources
    if (shared_state != NULL) {
        detach_shared_memory(shared_state);
    }
    
    if (shm_id != -1) {
        destroy_shared_memory(shm_id);
    }
    
    if (sem_id != -1) {
        destroy_semaphore_set(sem_id);
    }
    
    if (report_queue_id != -1) {
        destroy_report_queue(report_queue_id);
    }
    
    // Clean up prep message queues
    if (gang_pids != NULL) {
        for (int i = 0; i < shared_state->num_gangs; i++) {
            int prep_queue_id = msgget(REPORT_QUEUE_KEY + 1000 + i, 0666);
            if (prep_queue_id != -1) {
                msgctl(prep_queue_id, IPC_RMID, NULL);
            }
        }
    }
    
    // Free allocated memory
    if (gang_pids != NULL) {
        free(gang_pids);
    }
    
    // Free visualization resources
    if (viz_context.gang_states != NULL) {
        free(viz_context.gang_states);
    }
    
    // Destroy mutex
    pthread_mutex_destroy(&viz_context.mutex);
    
    printf("Simulation terminated, resources cleaned up.\n");
}

// Signal handler for termination
void signal_handler(int sig) {
    printf("Received signal %d, terminating simulation...\n", sig);
    
    // Update shared state to stop the simulation
    if (shared_state != NULL) {
        shared_state->simulation_running = false;
    }
    
    // Kill all child processes if we're in the parent
    if (gang_pids != NULL) {
        for (int i = 0; i < config.max_gangs; i++) {
            if (gang_pids[i] > 0) {
                kill(gang_pids[i], SIGTERM);
            }
        }
        
        if (police_pid > 0) {
            kill(police_pid, SIGTERM);
        }
    }
    
    // Wait for all processes to terminate
    while (wait(NULL) > 0);
    
    // Clean up resources
    cleanup();
    
    exit(0);
}

// Gang process main function
void run_gang_process(int gang_id, SimulationConfig config) {
    Gang gang;
    
    // Initialize gang
    int num_members = random_int(config.min_members_per_gang, config.max_members_per_gang);
    initialize_gang(&gang, gang_id, num_members, config.gang_ranks, config);
    
    // Set report queue ID
    gang.report_queue_id = report_queue_id;
    
    // Attach to shared memory
    SharedState* shm = attach_shared_memory(shm_id);
    
    // Plan initial mission
    plan_new_mission(&gang, config);
    
    // Track preparation time
    int time_spent_preparing = 0;
    bool mission_planned = true;
    
    // Main gang loop
    while (shm->simulation_running) {
        // Check if termination conditions are met
        if (shm->total_successful_missions >= config.max_successful_plans ||
            shm->total_thwarted_missions >= config.max_thwarted_plans ||
            shm->total_executed_agents >= config.max_executed_agents) {
            break;
        }
        
        // Check for arrest notification from police
        semaphore_wait(sem_id, 0);
        if (shm->gang_status[gang_id].is_arrested && !shm->gang_status[gang_id].arrest_notification_seen) {
            // Gang has been arrested - process notification
            gang.is_in_prison = true;
            gang.prison_time_remaining = shm->gang_status[gang_id].prison_time;
            shm->gang_status[gang_id].arrest_notification_seen = true;
            
            // Reset mission planning
            time_spent_preparing = 0;
            mission_planned = false;
            
            // Signal all gang member threads
            pthread_mutex_lock(&gang.gang_mutex);
            log_message("Gang %d has been arrested, %d members sent to prison for %d time units",
                       gang_id, gang.num_members, gang.prison_time_remaining);
            pthread_mutex_unlock(&gang.gang_mutex);
        }
        semaphore_signal(sem_id, 0);
        
        // Gang operations
        if (!gang.is_in_prison) {
            // Check if we're preparing or ready to execute
            if (mission_planned) {
                // Check if preparation time has elapsed
                if (time_spent_preparing >= gang.preparation_time) {
                    // Store previous mission counts to detect changes
                    int prev_successful = gang.successful_missions;
                    int prev_thwarted = gang.thwarted_missions;
                    int prev_executed = gang.executed_agents;
                    
                    // Execute mission
                    execute_mission(&gang, config);
                    
                    // Update shared memory based on mission outcome
                    semaphore_wait(sem_id, 0);
                    if (gang.successful_missions > prev_successful) {
                        shm->total_successful_missions++;
                        log_message("Gang %d mission succeeded - total successful missions: %d", 
                                   gang_id, shm->total_successful_missions);
                    }
                    if (gang.thwarted_missions > prev_thwarted) {
                        shm->total_thwarted_missions++;
                        log_message("Gang %d mission failed - total thwarted missions: %d", 
                                   gang_id, shm->total_thwarted_missions);
                    }
                    if (gang.executed_agents > prev_executed) {
                        shm->total_executed_agents += (gang.executed_agents - prev_executed);
                        log_message("Gang %d executed %d agents - total executed agents: %d", 
                                   gang_id, (gang.executed_agents - prev_executed), shm->total_executed_agents);
                    }
                    semaphore_signal(sem_id, 0);
                    
                    // Plan next mission
                    plan_new_mission(&gang, config);
                    time_spent_preparing = 0;
                } else {
                    // Continue preparing
                    time_spent_preparing++;
                    
                    // Log preparation status periodically
                    if (time_spent_preparing % 2 == 0) {
                        int total_prep = 0;
                        int max_possible_prep = 0;
                        pthread_mutex_lock(&gang.gang_mutex);
                        for (int i = 0; i < gang.num_members; i++) {
                            total_prep += gang.members[i].preparation_level;
                            max_possible_prep += gang.required_preparation_level;
                        }
                        // Calculate as percentage of required level
                        int avg_prep = max_possible_prep > 0 ? (total_prep * 100) / max_possible_prep : 0;
                        pthread_mutex_unlock(&gang.gang_mutex);
                        
                        log_message("Gang %d preparing for %s: %d/%d time units, %d%% prepared", 
                                   gang.id, crime_type_to_string(gang.current_target),
                                   time_spent_preparing, gang.preparation_time, avg_prep);
                        
                        // Send preparation level to visualization through message queue
                        int prep_queue_id = msgget(REPORT_QUEUE_KEY + 1000 + gang.id, IPC_CREAT | 0666);
                        if (prep_queue_id != -1) {
                            struct {
                                long mtype;
                                int preparation_level;
                                CrimeType current_target;
                                int num_members;
                            } prep_msg;
                            
                            prep_msg.mtype = 2; // Message type 2 for preparation updates
                            prep_msg.preparation_level = avg_prep;
                            prep_msg.current_target = gang.current_target;
                            prep_msg.num_members = gang.num_members;
                            
                            msgsnd(prep_queue_id, &prep_msg, sizeof(prep_msg) - sizeof(long), IPC_NOWAIT);
                        }
                    }
                    
                    // Sleep to simulate time passing and avoid busy waiting
                    usleep(500000); // Sleep for 0.5 seconds
                }
            } else {
                // Plan new mission if we don't have one
                plan_new_mission(&gang, config);
                time_spent_preparing = 0;
                mission_planned = true;
            }
        }
        else {
            // Gang is in prison, decrease prison time
            gang.prison_time_remaining--;
            if (gang.prison_time_remaining <= 0) {
                gang.is_in_prison = false;
                
                // Update shared memory to clear arrest status
                semaphore_wait(sem_id, 0);
                shm->gang_status[gang_id].is_arrested = false;
                semaphore_signal(sem_id, 0);
                
                log_message("Gang %d has been released from prison", gang_id);
                
                // Signal all gang member threads to resume operations
                pthread_mutex_lock(&gang.gang_mutex);
                pthread_cond_broadcast(&gang.gang_cond);
                pthread_mutex_unlock(&gang.gang_mutex);
            }
            sleep(1); // Sleep to avoid busy waiting
        }
    }
    
    // Cleanup
    cleanup_gang(&gang);
    detach_shared_memory(shm);
    exit(0);
}

// Police process main function
void run_police_process(SimulationConfig config) {
    Police police;
    
    // Initialize police
    initialize_police(&police, config);
    
    // Set report queue ID
    police.report_queue_id = report_queue_id;
    
    // Attach to shared memory
    SharedState* shm = attach_shared_memory(shm_id);
    
    // Create police thread
    pthread_t police_thread;
    pthread_create(&police_thread, NULL, police_routine, &police);
    
    // Main police loop
    while (shm->simulation_running) {
        // Check if termination conditions are met
        if (shm->total_successful_missions >= config.max_successful_plans ||
            shm->total_thwarted_missions >= config.max_thwarted_plans ||
            shm->total_executed_agents >= config.max_executed_agents) {
            break;
        }
        
        // Process intelligence and take action
        IntelligenceReport report;
        if (receive_report(report_queue_id, &report) > 0) {
            process_intelligence(&police, report, config);
            
            // Check if action should be taken
            if (decide_on_action(&police, report.gang_id, config)) {
                arrest_gang_members(&police, report.gang_id, config);
                
                // Update shared memory
                semaphore_wait(sem_id, 0);
                shm->total_thwarted_missions++;
                semaphore_signal(sem_id, 0);
            }
        }
        
        // Update shared memory with lost agents
        semaphore_wait(sem_id, 0);
        shm->total_executed_agents = police.lost_agents;
        semaphore_signal(sem_id, 0);
    }
    
    // Wait for police thread to finish
    pthread_join(police_thread, NULL);
    
    // Cleanup
    cleanup_police(&police);
    detach_shared_memory(shm);
    exit(0);
}

// Thread function for visualization loop
void* visualization_thread_func(void* arg) {
    printf("Starting visualization thread...\n");
    
    // Mark thread as running and initialize health counter
    pthread_mutex_lock(&viz_context.mutex);
    viz_context.viz_thread_running = true;
    viz_context.viz_thread_health = 1;
    pthread_mutex_unlock(&viz_context.mutex);
    
    // Check if DISPLAY environment is available
    char* display = getenv("DISPLAY");
    if (!display || strlen(display) == 0) {
        fprintf(stderr, "Warning: No DISPLAY environment variable set. Falling back to text-only mode.\n");
        // Text-only mode - enhanced version
        int frame = 0;
        while (1) {
            // Thread-safe access to simulation status
            bool keep_running;
            pthread_mutex_lock(&viz_context.mutex);
            keep_running = viz_context.simulation_running;
            viz_context.viz_thread_health++; // Increment health counter
            pthread_mutex_unlock(&viz_context.mutex);
            
            if (!keep_running) break;
            
            if (frame++ % 5 == 0) {  // Update every 5 frames
                // Clear screen and print header (ANSI escape sequences)
                printf("\033[2J\033[H");  // Clear screen and move cursor to top
                printf("===== Crime Simulation Text Visualization - Frame %d =====\n\n", frame);
                
                // Display gang information - thread-safe access
                pthread_mutex_lock(&viz_context.mutex);
                printf("Gangs:\n");
                for (int i = 0; i < viz_context.num_gangs; i++) {
                    if (viz_context.gang_states != NULL) {
                        printf("  Gang %d: %s\n", i, 
                            viz_context.gang_states[i].is_in_prison ? "In Prison" : "Active");
                        printf("    Members: %d, Agents: %d\n", 
                            viz_context.gang_states[i].num_members,
                            viz_context.gang_states[i].num_agents);
                        printf("    Preparation: %d%%\n", 
                            viz_context.gang_states[i].preparation_level);
                        printf("    Target: %s\n\n", 
                            crime_type_to_string(viz_context.gang_states[i].current_target));
                    }
                }
                
                // Display simulation statistics
                if (viz_context.shared_state != NULL) {
                    printf("\nStatistics:\n");
                    printf("  Successful missions: %d / %d\n", 
                        viz_context.shared_state->total_successful_missions,
                        viz_context.config.max_successful_plans);
                    printf("  Thwarted missions: %d / %d\n", 
                        viz_context.shared_state->total_thwarted_missions,
                        viz_context.config.max_thwarted_plans);
                    printf("  Executed agents: %d / %d\n", 
                        viz_context.shared_state->total_executed_agents,
                        viz_context.config.max_executed_agents);
                    printf("  Animation time: %.1f\n", 
                        viz_context.animation_time);
                }
                pthread_mutex_unlock(&viz_context.mutex);
            }
            usleep(viz_context.refresh_rate * 1000); // Convert ms to μs
        }
        
        // Mark thread as stopped before exiting
        pthread_mutex_lock(&viz_context.mutex);
        viz_context.viz_thread_running = false;
        pthread_mutex_unlock(&viz_context.mutex);
        return NULL;
    }
    
    // In graphical mode, this thread should only update data periodically,
    // NOT try to run the GLUT main loop (which should run in the main thread)
    printf("Visualization thread started in graphical mode, will update data only\n");
    
    int frame = 0;
    while (1) {
        // Thread-safe access to simulation status
        bool keep_running;
        pthread_mutex_lock(&viz_context.mutex);
        keep_running = viz_context.simulation_running;
        viz_context.viz_thread_health++; // Increment health counter
        
        // Update animation time
        viz_context.animation_time += 0.1f;
        pthread_mutex_unlock(&viz_context.mutex);
        
        if (!keep_running) break;
        
        // Request a redraw
        glutPostRedisplay();
        
        usleep(viz_context.refresh_rate * 1000); // Convert ms to μs
    }
    
    // Mark thread as stopped before exiting
    pthread_mutex_lock(&viz_context.mutex);
    viz_context.viz_thread_running = false;
    pthread_mutex_unlock(&viz_context.mutex);
    return NULL;
}

// Thread function for state updates
void* gang_state_update_thread(void* arg) {
    int num_gangs = shared_state->num_gangs;
    bool simulation_ended = false;
    
    // Process update loop that runs alongside glutMainLoop
    while (1) {  // Keep running even if simulation ends
        // Check if we've reached termination conditions
        bool sim_running = shared_state->simulation_running;
        
        // If simulation just ended, print a message
        if (sim_running == false && simulation_ended == false) {
            simulation_ended = true;
            printf("Simulation has ended, but visualization will continue\n");
        }
        
        // Update gang visualization states from shared memory
        for (int i = 0; i < num_gangs; i++) {
            pthread_mutex_lock(&viz_context.mutex);
            // Update arrest status
            viz_context.gang_states[i].is_in_prison = shared_state->gang_status[i].is_arrested;
            viz_context.gang_states[i].prison_time_remaining = shared_state->gang_status[i].prison_time;
            pthread_mutex_unlock(&viz_context.mutex);
            
            // Update preparation level - get this data through a message queue
            int msg_queue_id = msgget(REPORT_QUEUE_KEY + 1000 + i, 0666);
            if (msg_queue_id != -1) {
                // Try to receive an update message without blocking
                struct {
                    long mtype;
                    int preparation_level;
                    CrimeType current_target;
                    int num_members;
                } prep_msg;
                
                if (msgrcv(msg_queue_id, &prep_msg, sizeof(prep_msg) - sizeof(long), 2, IPC_NOWAIT) != -1) {
                    // Update visualization with thread safety
                    pthread_mutex_lock(&viz_context.mutex);
                    viz_context.gang_states[i].preparation_level = prep_msg.preparation_level;
                    viz_context.gang_states[i].current_target = prep_msg.current_target;
                    viz_context.gang_states[i].num_members = prep_msg.num_members;
                    pthread_mutex_unlock(&viz_context.mutex);
                    
                    // Only print updates occasionally to avoid console spam
                    static int update_count = 0;
                    if (update_count++ % 10 == 0) {
                        printf("Queue %d: Updated gang %d preparation: %d%%, target: %s, members: %d\n", 
                               msg_queue_id, i, prep_msg.preparation_level, 
                               crime_type_to_string(prep_msg.current_target), 
                               prep_msg.num_members);
                    }
                }
            }
        }
        
        // If we've reached termination conditions but need to keep the visualization alive
        if (simulation_ended) {
            // Occasionally generate new preparation values for the gangs to make visualization interesting
            static int update_cycle = 0;
            if (update_cycle++ % 50 == 0) {
                for (int i = 0; i < num_gangs; i++) {
                    // Only update gangs that aren't in prison
                    pthread_mutex_lock(&viz_context.mutex);
                    if (!viz_context.gang_states[i].is_in_prison) {
                        // Randomly change preparation level
                        viz_context.gang_states[i].preparation_level = random_int(5, 95);
                        
                        // Randomly change crime type
                        viz_context.gang_states[i].current_target = (CrimeType)random_int(0, NUM_CRIME_TYPES - 1);
                    }
                    pthread_mutex_unlock(&viz_context.mutex);
                }
            }
        }
        
        // Sleep to avoid busy waiting
        usleep(200000); // 0.2 seconds
    }
    return NULL;
}

int main(int argc, char* argv[]) {
    // Check command line arguments
    if (argc < 2) {
        printf("Usage: %s <config_file>\n", argv[0]);
        return 1;
    }
    
    // Load configuration
    config = load_config(argv[1]);
    print_config(config);
    
    // Initialize random seed
    srand(time(NULL));
    
    // Set up signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize IPC mechanisms
    shm_id = create_shared_memory();
    shared_state = attach_shared_memory(shm_id);
    shared_state->simulation_running = true;
    shared_state->total_successful_missions = 0;
    shared_state->total_thwarted_missions = 0;
    shared_state->total_executed_agents = 0;
    
    // Initialize gang status array
    for (int i = 0; i < 100; i++) {
        shared_state->gang_status[i].is_arrested = false;
        shared_state->gang_status[i].prison_time = 0;
        shared_state->gang_status[i].arrest_notification_seen = true;
    }
    
    sem_id = create_semaphore_set();
    report_queue_id = create_report_queue();
    
    // Determine number of gangs
    int num_gangs = random_int(config.min_gangs, config.max_gangs);
    shared_state->num_gangs = num_gangs;
    printf("Creating %d gangs for simulation.\n", num_gangs);
    
    // Allocate memory for gang PIDs
    gang_pids = (pid_t*)malloc(num_gangs * sizeof(pid_t));
    
    // Create gang processes
    for (int i = 0; i < num_gangs; i++) {
        pid_t pid = fork();
        
        if (pid < 0) {
            // Fork failed
            perror("Fork failed");
            signal_handler(SIGTERM);
            return 1;
        }
        else if (pid == 0) {
            // Child process (gang)
            run_gang_process(i, config);
            // Should not return
            exit(0);
        }
        else {
            // Parent process
            gang_pids[i] = pid;
        }
    }
    
    // Create police process
    police_pid = fork();
    
    if (police_pid < 0) {
        // Fork failed
        perror("Fork failed");
        signal_handler(SIGTERM);
        return 1;
    }
    else if (police_pid == 0) {
        // Child process (police)
        run_police_process(config);
        // Should not return
        exit(0);
    }
    
    // Initialize visualization 
    printf("Initializing visualization...\n");
    
    // Set up visualization context first
    viz_context.gangs = NULL;  // Will be populated by visualization from shared memory
    viz_context.num_gangs = num_gangs;
    viz_context.police = NULL; // Will be populated by visualization from shared memory
    viz_context.config = config;
    viz_context.simulation_running = true;
    viz_context.refresh_rate = config.visualization_refresh_rate;
    viz_context.shared_state = shared_state;
    viz_context.viz_thread_running = false;
    viz_context.viz_thread_health = 0;

    // Initialize mutex for thread-safe access to visualization data
    if (pthread_mutex_init(&viz_context.mutex, NULL) != 0) {
        perror("Failed to initialize visualization mutex");
        fprintf(stderr, "Warning: Visualization may experience synchronization issues\n");
        // Continue, don't terminate
    }
    
    // Check if we have a DISPLAY environment variable before trying OpenGL
    char* display = getenv("DISPLAY");
    if (display && strlen(display) > 0) {
        printf("Display found (%s). Initializing OpenGL visualization...\n", display);
        // Initialize OpenGL visualization
        initialize_visualization(&argc, argv, &viz_context);
    } else {
        printf("No display available. Will use text-only visualization.\n");
        // Skip OpenGL initialization - will use text mode
    }
    
    // Create thread for visualization loop
    pthread_t viz_thread;
    if (pthread_create(&viz_thread, NULL, visualization_thread_func, NULL) != 0) {
        perror("Failed to create visualization thread");
        fprintf(stderr, "Warning: Could not create visualization thread, continuing with text-only mode\n");
        // Don't terminate, continue with text-only mode
        viz_context.simulation_running = true;
    }
    
    // Give the visualization thread a moment to initialize
    usleep(200000);  // Increased to 200ms delay
    
    // Initialize gang states for visualization
    viz_context.gang_states = (GangVisState*)malloc(num_gangs * sizeof(GangVisState));
    if (viz_context.gang_states == NULL) {
        perror("Failed to allocate memory for gang visualization states");
        // Don't terminate, just log the error
        fprintf(stderr, "Warning: Visualization will be limited due to memory allocation failure\n");
    } else {
        printf("Allocated gang_states at %p for %d gangs\n", (void*)viz_context.gang_states, num_gangs);
    
        for (int i = 0; i < num_gangs; i++) {
            viz_context.gang_states[i].id = i;
            viz_context.gang_states[i].is_in_prison = false;
            viz_context.gang_states[i].prison_time_remaining = 0;
            viz_context.gang_states[i].preparation_level = 0;
            viz_context.gang_states[i].current_target = BANK_ROBBERY; // Default
            
            // Initialize with default members (will be updated later)
            viz_context.gang_states[i].num_members = random_int(config.min_members_per_gang, config.max_members_per_gang);
            viz_context.gang_states[i].num_agents = viz_context.gang_states[i].num_members / 3; // Estimate some agents
            viz_context.gang_states[i].is_active = true;
            
            printf("Initialized gang %d with %d members\n", i, viz_context.gang_states[i].num_members);
            
            // Create initial message queue for this gang
            int prep_queue_id = msgget(REPORT_QUEUE_KEY + 1000 + i, IPC_CREAT | 0666);
            if (prep_queue_id != -1) {
                struct {
                    long mtype;
                    int preparation_level;
                    CrimeType current_target;
                    int num_members;
                } prep_msg;
                
                prep_msg.mtype = 2; // Message type 2 for preparation updates
                prep_msg.preparation_level = 0;
                prep_msg.current_target = viz_context.gang_states[i].current_target;
                prep_msg.num_members = viz_context.gang_states[i].num_members;
                
                msgsnd(prep_queue_id, &prep_msg, sizeof(prep_msg) - sizeof(long), IPC_NOWAIT);
            }
        }
    }
    
    // Animation time
    viz_context.animation_time = 0.0f;
    
    // No need to create another visualization thread, we already created one above
    
    // Thread health check variables
    int previous_health_count = 0;
    int health_check_failures = 0;
    
    // Parent process main loop
    if (display && strlen(display) > 0) {
        // In graphical mode, we need to run updates in a thread before glutMainLoop
        pthread_t update_thread;
        if (pthread_create(&update_thread, NULL, gang_state_update_thread, NULL) != 0) {
            perror("Failed to create update thread");
        } else {
            pthread_detach(update_thread);
        }
        
        // In graphical mode, we need to run glutMainLoop in the main thread
        printf("Starting GLUT main loop in the main thread...\n");
        glutMainLoop();
        // This code is only reached if glutMainLoop() somehow returns
        printf("GLUT main loop exited. Terminating simulation...\n");
    } else {
        // Text-only mode, run the normal monitoring loop
        while (shared_state->simulation_running) {
            // Check visualization thread health every few iterations
            pthread_mutex_lock(&viz_context.mutex);
            int current_health = viz_context.viz_thread_health;
            bool thread_running = viz_context.viz_thread_running;
            pthread_mutex_unlock(&viz_context.mutex);
            
            // Health checking logic from original code...
            if (thread_running && current_health == previous_health_count) {
                health_check_failures++;
                printf("Warning: Visualization thread may be stalled (failure count: %d)\n", health_check_failures);
                
                // If health check fails too many times, try to restart visualization
                if (health_check_failures > 5) {
                    printf("Visualization thread appears to be stuck, attempting recovery...\n");
                    
                    // Mark the thread as not running so it will exit if it's actually still active
                    pthread_mutex_lock(&viz_context.mutex);
                    viz_context.simulation_running = false;
                    pthread_mutex_unlock(&viz_context.mutex);
                    
                    // Give it a moment to notice and exit
                    usleep(100000);
                    
                    // Now restart it by creating a new thread
                    pthread_mutex_lock(&viz_context.mutex);
                    viz_context.simulation_running = true;
                    viz_context.viz_thread_running = false;
                    pthread_mutex_unlock(&viz_context.mutex);
                    
                    pthread_t viz_thread;
                    if (pthread_create(&viz_thread, NULL, visualization_thread_func, NULL) != 0) {
                        perror("Failed to restart visualization thread");
                    } else {
                        // Detach so we don't need to join
                        pthread_detach(viz_thread);
                        printf("Visualization thread restarted\n");
                        health_check_failures = 0;
                    }
                }
            } else if (thread_running) {
                // Thread is running and health count updated, reset failure counter
                health_check_failures = 0;
            }
            previous_health_count = current_health;
            
            // Check if termination conditions are met
            if (shared_state->total_successful_missions >= config.max_successful_plans) {
                printf("Simulation ended: Gangs completed %d successful missions.\n", 
                      shared_state->total_successful_missions);
                break;
            }
            else if (shared_state->total_thwarted_missions >= config.max_thwarted_plans) {
                printf("Simulation ended: Police thwarted %d gang plans.\n", 
                      shared_state->total_thwarted_missions);
                break;
            }
            else if (shared_state->total_executed_agents >= config.max_executed_agents) {
                printf("Simulation ended: %d secret agents have been executed.\n", 
                      shared_state->total_executed_agents);
                break;
            }
            
            // Update gang visualization states from shared memory
            for (int i = 0; i < num_gangs; i++) {
                // Update arrest status
                viz_context.gang_states[i].is_in_prison = shared_state->gang_status[i].is_arrested;
                viz_context.gang_states[i].prison_time_remaining = shared_state->gang_status[i].prison_time;
                
                // Update preparation level - get this data through a message queue
                int msg_queue_id = msgget(REPORT_QUEUE_KEY + 1000 + i, 0666);
                if (msg_queue_id != -1) {
                    // Try to receive an update message without blocking
                    struct {
                        long mtype;
                        int preparation_level;
                        CrimeType current_target;
                        int num_members;
                    } prep_msg;
                    
                    if (msgrcv(msg_queue_id, &prep_msg, sizeof(prep_msg) - sizeof(long), 2, IPC_NOWAIT) != -1) {
                        // Update visualization if we received a message
                        viz_context.gang_states[i].preparation_level = prep_msg.preparation_level;
                        viz_context.gang_states[i].current_target = prep_msg.current_target;
                        viz_context.gang_states[i].num_members = prep_msg.num_members;
                        
                        // Only print updates occasionally to avoid console spam
                        static int update_count = 0;
                        if (update_count++ % 10 == 0) {
                            printf("Queue %d: Updated gang %d preparation: %d%%, target: %s, members: %d\n", 
                                   msg_queue_id, i, prep_msg.preparation_level, 
                                   crime_type_to_string(prep_msg.current_target), 
                                   prep_msg.num_members);
                        }
                    } else {
                        // Debug message when no message is available (print less frequently)
                        static int no_message_count = 0;
                        if (no_message_count++ % 100 == 0) {  // Reduced frequency to every 100th failure
                            printf("No message available for gang %d in queue %d (errno: %d)\n", 
                                   i, msg_queue_id, errno);
                        }
                    }
                } else {
                    // Only print occasionally to avoid console spam
                    static int queue_fail_count = 0;
                    if (queue_fail_count++ % 200 == 0) {  // Reduced frequency to every 200th failure
                        printf("Failed to get message queue for gang %d (key: %d, errno: %d)\n", 
                               i, REPORT_QUEUE_KEY + 1000 + i, errno);
                    }
                }
            }
            
            // Update animation time
            viz_context.animation_time += 0.1f;
            
            // Sleep to avoid busy waiting
            usleep(500000); // 0.5 seconds
        }
    }
    
    // Set the flag to indicate simulation is stopping
    shared_state->simulation_running = false;
    
    // Signal all child processes to terminate
    signal_handler(SIGTERM);
    
    // Wait for all child processes to terminate
    printf("Waiting for all processes to terminate...\n");
    while (wait(NULL) > 0);
    
    // Clean up resources
    cleanup();
    
    printf("Simulation completed successfully.\n");
    return 0;
}
