#ifndef VISUALIZATION_H
#define VISUALIZATION_H

#include <GL/glut.h>
#include <stdbool.h>
#include <pthread.h>  // Add this for pthread_mutex_t
#include "config.h"
#include "ipc.h"

// Forward declarations to avoid circular dependencies
struct Gang;
struct Police;

// Gang visualization state
typedef struct {
    int id;
    bool is_in_prison;
    int prison_time_remaining;
    int preparation_level;
    CrimeType current_target;
    int num_members;
    int num_agents;
    bool is_active;
} GangVisState;

// Visualization context structure
typedef struct {
    Gang* gangs;                 // Gangs in the simulation
    int num_gangs;               // Number of gangs
    Police* police;              // Police in the simulation
    SimulationConfig config;     // Simulation configuration
    bool simulation_running;     // Flag to indicate if simulation is running
    int refresh_rate;            // Refresh rate in milliseconds
    int window_width;            // Window width
    int window_height;           // Window height
    float animation_time;        // Time for animation purposes
    GangVisState* gang_states;   // Gang visualization states
    SharedState* shared_state;   // Shared state for IPC
    pthread_mutex_t mutex;       // Mutex for thread-safe access to visualization data
    bool viz_thread_running;     // Flag to indicate if visualization thread is running
    int viz_thread_health;       // Counter for thread health checks
    // M-3: Scrolling support for gang lists
    int gang_list_scroll;        // Current scroll position for gang list
    int target_list_scroll;      // Current scroll position for target list
    // M-2: Gang expansion to view member details
    bool* expanded_gangs;        // Array to track expanded/collapsed gangs
} VisualizationContext;

// Global visualization context
extern VisualizationContext viz_context;

// Function prototypes
void initialize_visualization(int* argc, char** argv, VisualizationContext* ctx);
void display_function();
void reshape_function(int width, int height);
void timer_function(int value);
void keyboard_function(unsigned char key, int x, int y); // M-2, M-3: Keyboard handler for scrolling and toggling
void special_key_function(int key, int x, int y);     // M-2, M-3: Special keys (arrows) for scrolling
void mouse_function(int button, int state, int x, int y); // F-2: Mouse handler for gang expansion
void passive_motion_function(int x, int y);             // F-6: Track mouse for hover effects
void idle_function();
void draw_gangs(VisualizationContext* ctx);
void draw_police(VisualizationContext* ctx);
void draw_stats(VisualizationContext* ctx);
void draw_status_bar(VisualizationContext* ctx);

// New dashboard layout functions - modified to take coordinates and dimensions
void draw_gang_list(int x, int y, int width, int height);
void draw_current_target(int x, int y, int width, int height);
void draw_counters(int x, int y, int width, int height);
void draw_debug_info(VisualizationContext* ctx);
void cleanup_visualization();

#endif /* VISUALIZATION_H */
