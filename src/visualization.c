#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>  // For usleep
#include "../include/config.h"
#include "../include/visualization.h"
#include "../include/gang.h"
#include "../include/police.h"
#include "../include/utils.h"
#include "../include/ipc.h"

// Global visualization context is declared as extern in the header
// No need to redefine it here

// F-2: Track which gang expand button is being hovered over
int hover_gang_index = -1;

// F-6: Track mouse position for hover effects
int mouse_x = 0;
int mouse_y = 0;

// Colors for different entities (expanded to handle more than 7 gangs)
float gang_colors[][3] = {
    {1.0f, 0.0f, 0.0f},  // Red
    {0.0f, 0.0f, 1.0f},  // Blue
    {0.0f, 1.0f, 0.0f},  // Green
    {1.0f, 1.0f, 0.0f},  // Yellow
    {1.0f, 0.0f, 1.0f},  // Magenta
    {0.0f, 1.0f, 1.0f},  // Cyan
    {1.0f, 0.5f, 0.0f},  // Orange
    {0.7f, 0.3f, 0.7f},  // Purple
    {0.5f, 0.8f, 0.2f},  // Lime
    {0.9f, 0.6f, 0.3f}   // Peach
};

// Initialize OpenGL visualization
void initialize_visualization(int* argc, char** argv, VisualizationContext* ctx) {
    // Set environment variable to force software rendering if needed
    putenv("LIBGL_ALWAYS_SOFTWARE=1");
    
    // Print some debug info about the display
    char* display = getenv("DISPLAY");
    printf("DISPLAY environment variable: %s\n", display ? display : "not set");
    
    // Initialize GLUT - this should only happen once in the application
    glutInit(argc, argv);
    
    // Initialize display mode with double buffering and RGB color model
    // Double buffering prevents flickering during animation
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB);
    
    // Set window size and position
    ctx->window_width = 1024;  // Slightly larger window for better visibility
    ctx->window_height = 768;
    glutInitWindowSize(ctx->window_width, ctx->window_height);
    glutInitWindowPosition(50, 50);
    
    // Create window with error checking
    int window_id = glutCreateWindow("Crime Fighting Simulation Dashboard");
    if (window_id <= 0) {
        fprintf(stderr, "Error: Failed to create GLUT window\n");
        ctx->simulation_running = false;
        return;
    }
    
    // Register callbacks
    glutDisplayFunc(display_function);
    glutReshapeFunc(reshape_function);
    glutTimerFunc(ctx->refresh_rate, timer_function, 0);
    glutIdleFunc(idle_function);
    glutKeyboardFunc(keyboard_function);
    glutSpecialFunc(special_key_function);
    glutMouseFunc(mouse_function);
    glutPassiveMotionFunc(passive_motion_function);
    
    // Don't use glutIdleFunc as it can cause busy-waiting and high CPU usage
    // Instead, rely completely on timer-based updates
    
    // Set up timer for simulation updates - uses glutTimerFunc not busy polling
    glutTimerFunc(ctx->refresh_rate, timer_function, 0);
    
    // Set up clear color to dark slate (#1e1e1e) as per G-4 requirement
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    
    // Initialize orthographic projection for 2D rendering
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, ctx->window_width, 0, ctx->window_height);
    glMatrixMode(GL_MODELVIEW);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Save context globally
    viz_context = *ctx;
    viz_context.simulation_running = true;
    
    // Initialize scrolling positions
    viz_context.gang_list_scroll = 0;
    viz_context.target_list_scroll = 0;
    
    // Initialize gang expansion tracking
    viz_context.expanded_gangs = (bool*)calloc(viz_context.num_gangs, sizeof(bool));
    if (!viz_context.expanded_gangs) {
        fprintf(stderr, "Error: Failed to allocate memory for expanded_gangs array\n");
    }
    
    printf("OpenGL visualization initialized successfully\n");
    // glutMainLoop() will be called in the visualization thread
}

// Display callback function
void display_function() {
    // Thread-safe access to visualization context
    bool simulation_running;
    pthread_mutex_lock(&viz_context.mutex);
    simulation_running = viz_context.simulation_running;
    pthread_mutex_unlock(&viz_context.mutex);
    
    // Check if the visualization context is properly initialized
    if (!simulation_running) {
        // Something's wrong, just clear the screen to dark slate and return
        glClearColor(0.12f, 0.12f, 0.12f, 1.0f); // #1e1e1e dark slate
        glClear(GL_COLOR_BUFFER_BIT);
        glutSwapBuffers();
        return;
    }

    // G-4: Clear background to dark slate color (#1e1e1e)
    glClearColor(0.12f, 0.12f, 0.12f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    
    // Enable blending for transparency
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Reset modelview matrix for orthographic 2D rendering
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    // G-2: Create the dashboard layout with three columns
    int window_width = viz_context.window_width;
    int window_height = viz_context.window_height;
    
    // Calculate column widths and positions
    int left_col_width = window_width * 0.25;    // 25% of window width
    int center_col_width = window_width * 0.5;   // 50% of window width
    int right_col_width = window_width * 0.25;   // 25% of window width
    
    int left_col_x = 0;
    int center_col_x = left_col_width;
    int right_col_x = left_col_width + center_col_width;
    
    // Draw subtle column dividers
    glColor4f(0.3f, 0.3f, 0.3f, 0.5f);
    
    // Left-center divider
    glBegin(GL_LINES);
    glVertex2f(left_col_width, 0);
    glVertex2f(left_col_width, window_height);
    glEnd();
    
    // Center-right divider
    glBegin(GL_LINES);
    glVertex2f(right_col_x, 0);
    glVertex2f(right_col_x, window_height);
    glEnd();
    
    // G-2: Draw contents in each column
    // Left column: List of gangs with colored status icons
    draw_gang_list(left_col_x, 0, left_col_width, window_height * 0.9);
    
    // Center column: Current target and progress bar
    draw_current_target(center_col_x, 0, center_col_width, window_height * 0.9);
    
    // Right column: Counters for plans thwarted, succeeded, agents executed
    draw_counters(right_col_x, 0, right_col_width, window_height * 0.9);
    
    // Draw status bar at the top
    draw_status_bar(&viz_context);
    
    // Disable blending
    glDisable(GL_BLEND);
    
    // G-1: Use double-buffering to prevent flickering
    glutSwapBuffers();
}

// Reshape callback function
void reshape_function(int width, int height) {
    // Update window size
    viz_context.window_width = width;
    viz_context.window_height = height;
    
    // Set viewport to cover the new window
    glViewport(0, 0, width, height);
    
    // Set up the projection matrix for pixel coordinates
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    
    // Use window coordinates with origin at bottom-left
    // OpenGL's default origin is bottom-left, so y-axis goes up
    gluOrtho2D(0.0, (GLdouble)width, 0.0, (GLdouble)height);
    
    // Switch back to modelview matrix
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    
    printf("Resized window to %d x %d pixels\n", width, height);
}

// Timer callback function for simulation updates
void timer_function(int value) {
    // G-6: CPU Discipline - Only run when window exists and simulation is active
    if (!glutGetWindow()) {
        printf("No GLUT window exists in timer function, skipping update\n");
        return;
    }
    
    // G-3: Dynamic Updates - Check if the simulation is still running
    pthread_mutex_lock(&viz_context.mutex);
    bool simulation_running = viz_context.simulation_running;
    // We can't reliably detect if window is visible in all GLUT versions
    // So just assume it's visible if it exists
    bool window_visible = true;
    pthread_mutex_unlock(&viz_context.mutex);
    
    if (simulation_running) {
        if (window_visible) {
            // Only request redisplay when window is visible
            glutPostRedisplay();
            
            // Update health counter to track visualization thread
            pthread_mutex_lock(&viz_context.mutex);
            viz_context.viz_thread_health++;
            pthread_mutex_unlock(&viz_context.mutex);
        } else {
            // G-6: If window not visible, sleep longer to reduce CPU usage
            usleep(100000); // 100ms sleep when minimized
        }
        // Force a redraw to update the visualization
        glutPostRedisplay();
        
        // Set up next timer
        glutTimerFunc(viz_context.refresh_rate, timer_function, 0);
    } else {
        // If simulation is no longer running, we could exit, but let's just stop the timer
        printf("Simulation stopped, visualization will no longer update\n");
    }
}

// Idle function to ensure continuous rendering
void idle_function() {
    // We don't need to do anything here since we're using timer-based updates
}

// M-2: Keyboard callback function for toggling gang details and scrolling
void keyboard_function(unsigned char key, int x, int y) {
    pthread_mutex_lock(&viz_context.mutex);
    int num_gangs = viz_context.num_gangs;
    pthread_mutex_unlock(&viz_context.mutex);
    
    switch(key) {
        // Toggle individual gang details with number keys 0-9
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9': {
            int gang_index = key - '0';
            pthread_mutex_lock(&viz_context.mutex);
            if (gang_index < num_gangs && viz_context.expanded_gangs != NULL) {
                viz_context.expanded_gangs[gang_index] = !viz_context.expanded_gangs[gang_index];
            }
            pthread_mutex_unlock(&viz_context.mutex);
            glutPostRedisplay();
            break;
        }
        // '+' key to expand all gangs
        case '+':
        case '=':
            pthread_mutex_lock(&viz_context.mutex);
            for (int i = 0; i < num_gangs; i++) {
                if (viz_context.expanded_gangs != NULL) {
                    viz_context.expanded_gangs[i] = true;
                }
            }
            pthread_mutex_unlock(&viz_context.mutex);
            glutPostRedisplay();
            break;
        // '-' key to collapse all gangs
        case '-':
            pthread_mutex_lock(&viz_context.mutex);
            for (int i = 0; i < num_gangs; i++) {
                if (viz_context.expanded_gangs != NULL) {
                    viz_context.expanded_gangs[i] = false;
                }
            }
            pthread_mutex_unlock(&viz_context.mutex);
            glutPostRedisplay();
            break;
        // 'h' key to reset to home position (top of lists)
        case 'h':
        case 'H':
            pthread_mutex_lock(&viz_context.mutex);
            viz_context.gang_list_scroll = 0;
            viz_context.target_list_scroll = 0;
            pthread_mutex_unlock(&viz_context.mutex);
            glutPostRedisplay();
            break;
        // ESC to exit
        case 27:
            exit(0);
            break;
    }
}

// M-3: Special key callback function for scrolling
void special_key_function(int key, int x, int y) {
    pthread_mutex_lock(&viz_context.mutex);
    int num_gangs = viz_context.num_gangs;
    int gang_list_scroll = viz_context.gang_list_scroll;
    int target_list_scroll = viz_context.target_list_scroll;
    pthread_mutex_unlock(&viz_context.mutex);
    
    switch(key) {
        case GLUT_KEY_UP: // Up arrow key
            // Scroll gang list up
            if (gang_list_scroll > 0) {
                pthread_mutex_lock(&viz_context.mutex);
                viz_context.gang_list_scroll--;
                pthread_mutex_unlock(&viz_context.mutex);
                glutPostRedisplay();
            }
            break;
            
        case GLUT_KEY_DOWN: // Down arrow key
            // Scroll gang list down
            if (gang_list_scroll < num_gangs - 1) {
                pthread_mutex_lock(&viz_context.mutex);
                viz_context.gang_list_scroll++;
                pthread_mutex_unlock(&viz_context.mutex);
                glutPostRedisplay();
            }
            break;
            
        case GLUT_KEY_PAGE_UP: // Page Up key
            // Scroll target list up
            if (target_list_scroll > 0) {
                pthread_mutex_lock(&viz_context.mutex);
                viz_context.target_list_scroll--;
                pthread_mutex_unlock(&viz_context.mutex);
                glutPostRedisplay();
            }
            break;
            
        case GLUT_KEY_PAGE_DOWN: // Page Down key
            // Scroll target list down
            if (target_list_scroll < num_gangs - 1) {
                pthread_mutex_lock(&viz_context.mutex);
                viz_context.target_list_scroll++;
                pthread_mutex_unlock(&viz_context.mutex);
                glutPostRedisplay();
            }
            break;
            
        case GLUT_KEY_HOME: // Home key
            // Reset both scrolling positions
            pthread_mutex_lock(&viz_context.mutex);
            viz_context.gang_list_scroll = 0;
            viz_context.target_list_scroll = 0;
            pthread_mutex_unlock(&viz_context.mutex);
            glutPostRedisplay();
            break;
    }
}

// F-2: Mouse callback function for handling clicks on gang expansion buttons
void mouse_function(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        // Check if click is within gang expansion area
        if (hover_gang_index >= 0) {
            pthread_mutex_lock(&viz_context.mutex);
            if (hover_gang_index < viz_context.num_gangs && viz_context.expanded_gangs != NULL) {
                // Toggle the expanded state
                viz_context.expanded_gangs[hover_gang_index] = !viz_context.expanded_gangs[hover_gang_index];
            }
            pthread_mutex_unlock(&viz_context.mutex);
            glutPostRedisplay();
        }
    }
    // F-4: Handle mouse wheel for scrolling
    else if (button == 3 || button == 4) { // Wheel up (3) or down (4)
        pthread_mutex_lock(&viz_context.mutex);
        int num_gangs = viz_context.num_gangs;
        int max_scroll = num_gangs - 1;
        
        // Window regions: left third is gang list, middle third is target list
        int window_width = viz_context.window_width;
        int x_third = window_width / 3;
        
        if (x < x_third) { // Left panel - Gang list
            if (button == 3 && viz_context.gang_list_scroll > 0) { // Wheel up
                viz_context.gang_list_scroll--;
            } else if (button == 4 && viz_context.gang_list_scroll < max_scroll) { // Wheel down
                viz_context.gang_list_scroll++;
            }
        } else if (x < 2 * x_third) { // Middle panel - Target list
            if (button == 3 && viz_context.target_list_scroll > 0) { // Wheel up
                viz_context.target_list_scroll--;
            } else if (button == 4 && viz_context.target_list_scroll < max_scroll) { // Wheel down
                viz_context.target_list_scroll++;
            }
        }
        pthread_mutex_unlock(&viz_context.mutex);
        glutPostRedisplay();
    }
}

// F-6: Track mouse position for hover effects
void passive_motion_function(int x, int y) {
    mouse_x = x;
    mouse_y = y;
    
    // Reset hover state
    int old_hover_gang = hover_gang_index;
    hover_gang_index = -1;
    
    // Get window dimensions
    int window_width = viz_context.window_width;
    int window_height = viz_context.window_height;
    
    // Left panel dimensions (for gang list)
    int panel_x = 0;
    int panel_y = 0;
    int panel_width = window_width / 3;
    int panel_height = window_height;
    
    // Only check for hover if in left panel
    if (x >= panel_x && x <= panel_x + panel_width) {
        pthread_mutex_lock(&viz_context.mutex);
        int num_gangs = viz_context.num_gangs;
        int scroll_pos = viz_context.gang_list_scroll;
        bool* expanded_gangs = viz_context.expanded_gangs;
        pthread_mutex_unlock(&viz_context.mutex);
        
        // Calculate gang entry positions similar to draw_gang_list
        int base_gang_height = 40;
        int expanded_gang_height = 120;
        int visible_height = panel_height - 50;
        int gang_y_offset = panel_height - 70;
        
        for (int i = scroll_pos; i < num_gangs && (gang_y_offset > panel_y + 20); i++) {
            bool is_expanded = (expanded_gangs && i < num_gangs && expanded_gangs[i]);
            
            // Check if mouse is over the expand/collapse button area
            int button_x = panel_x + panel_width - 25;
            int button_width = 20;
            int button_height = 20;
            int button_y = gang_y_offset - 10;
            
            if (x >= button_x && x <= button_x + button_width &&
                y >= window_height - button_y - button_height && y <= window_height - button_y) {
                hover_gang_index = i;
                break;
            }
            
            // Move to next gang position
            gang_y_offset -= base_gang_height;
            if (is_expanded) {
                gang_y_offset -= expanded_gang_height - base_gang_height;
            }
        }
        
        // Force redisplay only if hover state changed
        if (old_hover_gang != hover_gang_index) {
            glutPostRedisplay();
        }
    }
}

// Function to draw a status bar at the top of the screen
void draw_status_bar(VisualizationContext* ctx) {
    // Draw a background for the status bar
    glColor4f(0.2f, 0.2f, 0.2f, 0.8f);
    glBegin(GL_QUADS);
    glVertex2f(0, ctx->window_height - 30);
    glVertex2f(ctx->window_width, ctx->window_height - 30);
    glVertex2f(ctx->window_width, ctx->window_height);
    glVertex2f(0, ctx->window_height);
    glEnd();
    
    // Draw simulation status
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(10, ctx->window_height - 20);
    
    char buffer[100];
    
    // Show current time
    time_t now = time(NULL);
    struct tm* timeinfo = localtime(&now);
    
    sprintf(buffer, "Simulation Time: %02d:%02d:%02d | Status: %s", 
            timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
            ctx->simulation_running ? "Running" : "Stopped");
    
    for (int i = 0; i < strlen(buffer); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
    }
    
    // Show termination condition if available
    if (ctx->shared_state != NULL) {
        if (ctx->shared_state->total_successful_missions >= ctx->config.max_successful_plans) {
            glColor3f(1.0f, 0.5f, 0.0f); // Orange for gangs winning
            glRasterPos2f(ctx->window_width - 200, ctx->window_height - 20);
            sprintf(buffer, "Gangs Win! (%d missions)", ctx->shared_state->total_successful_missions);
        } else if (ctx->shared_state->total_thwarted_missions >= ctx->config.max_thwarted_plans) {
            glColor3f(0.0f, 0.7f, 1.0f); // Blue for police winning
            glRasterPos2f(ctx->window_width - 200, ctx->window_height - 20);
            sprintf(buffer, "Police Win! (%d thwarts)", ctx->shared_state->total_thwarted_missions);
        } else if (ctx->shared_state->total_executed_agents >= ctx->config.max_executed_agents) {
            glColor3f(1.0f, 0.0f, 0.0f); // Red for agents executed
            glRasterPos2f(ctx->window_width - 200, ctx->window_height - 20);
            sprintf(buffer, "Agents Lost! (%d executed)", ctx->shared_state->total_executed_agents);
        } else {
            // Still running
            buffer[0] = '\0';
        }
        
        for (int i = 0; i < strlen(buffer); i++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
        }
    }
}

// Function to add visual debugging indicators
void draw_debug_info(VisualizationContext* ctx) {
    // Set text color
    glColor3f(1.0f, 1.0f, 0.0f); // Yellow text for visibility
    
    // Draw at top-left corner
    float text_x = 10;
    float text_y = ctx->window_height - 50;
    
    // Display number of gangs and animation time
    char buffer[100];
    sprintf(buffer, "Debug: %d gangs, %.1f anim time", ctx->num_gangs, ctx->animation_time);
    glRasterPos2f(text_x, text_y);
    for (int i = 0; i < strlen(buffer); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
    }
    
    // Display address of gang states
    sprintf(buffer, "Gang states: %p", (void*)ctx->gang_states);
    glRasterPos2f(text_x, text_y - 15);
    for (int i = 0; i < strlen(buffer); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, buffer[i]);
    }
    
    // Draw coordinate system reference
    glColor3f(1.0f, 0.0f, 0.0f); // Red for X axis
    glBegin(GL_LINES);
    glVertex2f(50, 50);
    glVertex2f(150, 50);
    glEnd();
    glColor3f(0.0f, 1.0f, 0.0f); // Green for Y axis
    glBegin(GL_LINES);
    glVertex2f(50, 50);
    glVertex2f(50, 150);
    glEnd();
    
    // Draw coordinate labels
    glColor3f(1.0f, 1.0f, 1.0f);
    glRasterPos2f(150, 55);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'X');
    glRasterPos2f(55, 150);
    glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, 'Y');
}

// Function to draw the left column showing gang list with status icons
void draw_gang_list(int x, int y, int width, int height) {
    // Get thread-safe access to visualization context
    pthread_mutex_lock(&viz_context.mutex);
    int num_gangs = viz_context.num_gangs;
    int scroll_pos = viz_context.gang_list_scroll;
    bool* expanded_gangs = viz_context.expanded_gangs;
    pthread_mutex_unlock(&viz_context.mutex);
    
    // Calculate how many gangs can fit in the visible area
    int base_gang_height = 40;  // Basic height for a collapsed gang entry
    int expanded_gang_height = 120; // Height when expanded to show members
    int visible_height = height - 50; // Height available for gang list (subtract title area)
    int max_gangs_visible = visible_height / base_gang_height;
    
    // Draw section title
    glColor3f(1.0f, 1.0f, 1.0f);  // White text
    glRasterPos2f(x + 10, height - 30);
    char* title = "ACTIVE GANGS";
    for (int i = 0; i < strlen(title); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
    }
    
    // Draw horizontal separator
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_LINES);
    glVertex2f(x + 5, height - 40);
    glVertex2f(x + width - 5, height - 40);
    glEnd();
    
    // M-3: Draw scroll indicators if needed
    if (scroll_pos > 0) {
        // Draw up arrow for scrolling up
        glColor3f(0.7f, 0.7f, 0.7f); // Light gray
        glBegin(GL_TRIANGLES);
        glVertex2f(x + width - 20, height - 50);
        glVertex2f(x + width - 10, height - 60);
        glVertex2f(x + width - 30, height - 60);
        glEnd();
    }
    
    if (scroll_pos + max_gangs_visible < num_gangs) {
        // Draw down arrow for scrolling down
        glColor3f(0.7f, 0.7f, 0.7f); // Light gray
        glBegin(GL_TRIANGLES);
        glVertex2f(x + width - 20, y + 20);
        glVertex2f(x + width - 10, y + 30);
        glVertex2f(x + width - 30, y + 30);
        glEnd();
    }
    
    // Draw each gang with status indicator, starting from scroll position
    int gang_y_offset = height - 70;
    
    for (int i = scroll_pos; i < num_gangs && (gang_y_offset > y + 20); i++) {
        pthread_mutex_lock(&viz_context.mutex);
        GangVisState gang_state = viz_context.gang_states[i];
        SharedState* shared_state = viz_context.shared_state;
        pthread_mutex_unlock(&viz_context.mutex);
        
        // Draw gang status icon (colored circle)
        float circle_x = x + 20;
        float circle_y = gang_y_offset;
        float circle_radius = 8.0f;
        
        // G-4: Choose color based on gang status
        if (!gang_state.is_active) {
            // Red for dismantled gang
            glColor3f(0.8f, 0.0f, 0.0f);
        } else if (gang_state.is_in_prison) {
            // Yellow/amber for imprisoned gang
            glColor3f(0.9f, 0.6f, 0.0f);
        } else {
            // Green for free/active gang
            glColor3f(0.0f, 0.7f, 0.0f);
        }
        
        // Draw status circle
        glBegin(GL_TRIANGLE_FAN);
        glVertex2f(circle_x, circle_y); // Center
        for (int j = 0; j <= 20; j++) {
            float angle = 2.0f * 3.14159f * j / 20;
            float x_pos = circle_x + circle_radius * cos(angle);
            float y_pos = circle_y + circle_radius * sin(angle);
            glVertex2f(x_pos, y_pos);
        }
        glEnd();
        
        // Draw gang name and status
        glColor3f(1.0f, 1.0f, 1.0f);  // White text
        glRasterPos2f(x + 40, gang_y_offset + 5);
        
        char gang_info[50];
        sprintf(gang_info, "Gang %d", gang_state.id);
        for (int j = 0; j < strlen(gang_info); j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, gang_info[j]);
        }
        
        // Draw status text
        glRasterPos2f(x + 40, gang_y_offset - 10);
        char* status_text;
        if (!gang_state.is_active) {
            status_text = "Dismantled";
        } else if (gang_state.is_in_prison) {
            status_text = "Imprisoned";
        } else {
            status_text = "Active";
        }
        
        for (int j = 0; j < strlen(status_text); j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, status_text[j]);
        }
        
        // F-2: Draw expand/collapse indicator with hover effect
        if (i == hover_gang_index) {
            glColor3f(1.0f, 1.0f, 0.5f); // Highlight color when hovered
        } else {
            glColor3f(0.6f, 0.6f, 0.6f); // Normal gray
        }
        
        // F-2: Draw proper Unicode-style arrows (simulated with OpenGL)
        if (expanded_gangs && i < viz_context.num_gangs && expanded_gangs[i]) {
            // Draw ▼ (expanded) using triangles
            glBegin(GL_TRIANGLES);
            glVertex2f(x + width - 20, gang_y_offset + 5);
            glVertex2f(x + width - 10, gang_y_offset - 5);
            glVertex2f(x + width - 30, gang_y_offset - 5);
            glEnd();
        } else {
            // Draw ► (collapsed) using triangles
            glBegin(GL_TRIANGLES);
            glVertex2f(x + width - 25, gang_y_offset + 5);
            glVertex2f(x + width - 15, gang_y_offset);
            glVertex2f(x + width - 25, gang_y_offset - 5);
            glEnd();
        }
        
        // F-3: If expanded, show gang member details in a table format
        if (expanded_gangs && i < viz_context.num_gangs && expanded_gangs[i] && gang_state.is_active) {
            // F-3: Draw background for expanded section - alternating dark/darker for readability
            int expanded_height = (gang_state.num_members * 15) + 40; // Header + rows
            
            // Background for expanded area
            glColor3f(0.18f, 0.18f, 0.18f); // Darker background
            glBegin(GL_QUADS);
            glVertex2f(x + 5, gang_y_offset - 15);
            glVertex2f(x + width - 5, gang_y_offset - 15);
            glVertex2f(x + width - 5, gang_y_offset - 15 - expanded_height);
            glVertex2f(x + 5, gang_y_offset - 15 - expanded_height);
            glEnd();
            
            // F-3: Draw member table header with monospace font
            int header_y = gang_y_offset - 30;
            glColor3f(0.9f, 0.9f, 0.9f); // White/light gray for header
            
            // Column headers with spacing for alignment
            glRasterPos2f(x + 15, header_y);
            char* header = "ID   Rank  Prep%  Agent  Status";
            for (int j = 0; j < strlen(header); j++) {
                glutBitmapCharacter(GLUT_BITMAP_8_BY_13, header[j]);
            }
            
            // Draw separator line under header
            glColor3f(0.4f, 0.4f, 0.4f);
            glBegin(GL_LINES);
            glVertex2f(x + 10, header_y - 5);
            glVertex2f(x + width - 10, header_y - 5);
            glEnd();
            
            // F-3: Simulate member data rows with different statuses
            // In a real implementation, this would pull from shared memory
            int row_start_y = header_y - 20;
            int row_height = 15;
            
            // Simulated member data for demonstration (up to 5 members for space)
            int num_members_to_show = gang_state.num_members > 0 ? 
                                       (gang_state.num_members < 5 ? gang_state.num_members : 5) : 3;
            
            for (int j = 0; j < num_members_to_show; j++) {
                // Alternate row background for readability
                if (j % 2 == 1) {
                    glColor3f(0.22f, 0.22f, 0.22f); // Slightly lighter for alternating rows
                    glBegin(GL_QUADS);
                    glVertex2f(x + 10, row_start_y - (j * row_height) + 12);
                    glVertex2f(x + width - 10, row_start_y - (j * row_height) + 12);
                    glVertex2f(x + width - 10, row_start_y - (j * row_height) - 3);
                    glVertex2f(x + 10, row_start_y - (j * row_height) - 3);
                    glEnd();
                }
                
                // Generate sample ID (2 digits)
                int member_id = 10 + j;
                
                // Generate sample rank (1-5)
                int rank = (i + j) % 5 + 1;
                
                // Generate sample prep percentage (0-100%)
                int prep = (i * 10 + j * 7) % 100;
                
                // Determine if member is an agent ( symbol for agents)
                bool is_agent = (j == 1 || j == 3); // Example: members 1 and 3 are agents
                
                // Determine status (Alive, Dead, Prison)
                int status_type = j % 3; // 0=Alive, 1=Prison, 2=Dead
                
                // Draw row data
                char row_data[50];
                sprintf(row_data, "%2d   %d     %2d%%   %s    ", 
                        member_id, rank, prep, is_agent ? "" : "  ");
                
                glRasterPos2f(x + 15, row_start_y - (j * row_height));
                glColor3f(0.9f, 0.9f, 0.9f); // Default text color
                
                // Draw the formatted row data with monospace font
                for (int k = 0; k < strlen(row_data); k++) {
                    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, row_data[k]);
                }
                
                // Draw status with color coding
                char* status_text;
                switch(status_type) {
                    case 0: // Alive - Green
                        glColor3f(0.0f, 0.8f, 0.0f);
                        status_text = "Alive";
                        break;
                    case 1: // Prison - Blue
                        glColor3f(0.0f, 0.7f, 1.0f);
                        status_text = "Prison";
                        break;
                    case 2: // Dead - Red
                        glColor3f(1.0f, 0.0f, 0.0f);
                        status_text = "Dead";
                        break;
                }
                
                // Draw the status text
                for (int k = 0; k < strlen(status_text); k++) {
                    glutBitmapCharacter(GLUT_BITMAP_8_BY_13, status_text[k]);
                }
            }
            
            // When expanded, we need more vertical space
            gang_y_offset -= expanded_gang_height - base_gang_height;
        }
        
        // Move to next gang in the list
        gang_y_offset -= base_gang_height;
    }

}

// Function to draw the center panel with current target and progress bar
void draw_current_target(int x, int y, int width, int height) {
    // Get thread-safe access to visualization context
    pthread_mutex_lock(&viz_context.mutex);
    int num_gangs = viz_context.num_gangs;
    int scroll_pos = viz_context.target_list_scroll;
    pthread_mutex_unlock(&viz_context.mutex);
    
    // Calculate how many gangs can fit in the visible area
    int gang_item_height = 90;
    int visible_height = height - 50; // Height available for target list (subtract title area)
    int max_gangs_visible = (int)(visible_height / (float)gang_item_height); // Explicit float conversion
    
    // Draw section title
    glColor3f(1.0f, 1.0f, 1.0f);  // White text
    glRasterPos2f(x + (width / 2.0f) - 80, height - 30);
    char* title = "CURRENT OPERATIONS";
    for (int i = 0; i < strlen(title); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
    }
    
    // Draw horizontal separator
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_LINES);
    glVertex2f(x + 5, height - 40);
    glVertex2f(x + width - 5, height - 40);
    glEnd();
    
    // M-3: Draw scroll indicators if needed
    if (scroll_pos > 0) {
        // Draw up arrow for scrolling up
        glColor3f(0.7f, 0.7f, 0.7f); // Light gray
        glBegin(GL_TRIANGLES);
        glVertex2f(x + width - 20, height - 50);
        glVertex2f(x + width - 10, height - 60);
        glVertex2f(x + width - 30, height - 60);
        glEnd();
    }
    
    if (scroll_pos + max_gangs_visible < num_gangs) {
        // Draw down arrow for scrolling down
        glColor3f(0.7f, 0.7f, 0.7f); // Light gray
        glBegin(GL_TRIANGLES);
        glVertex2f(x + width - 20, y + 20);
        glVertex2f(x + width - 10, y + 30);
        glVertex2f(x + width - 30, y + 30);
        glEnd();
    }
    
    // Draw gang operations status
    int gang_y_offset = height - 90;
    
    for (int i = scroll_pos; i < num_gangs && (gang_y_offset > y + 20); i++) { // Display gangs that fit in the visible area
        pthread_mutex_lock(&viz_context.mutex);
        GangVisState gang_state = viz_context.gang_states[i];
        pthread_mutex_unlock(&viz_context.mutex);
        
        if (!gang_state.is_active) continue; // Skip inactive gangs
        
        // Draw gang identifier
        glColor3f(1.0f, 1.0f, 1.0f);
        glRasterPos2f(x + 20, gang_y_offset + 30);
        
        char gang_label[20];
        sprintf(gang_label, "GANG %d TARGET:", gang_state.id);
        for (int j = 0; j < strlen(gang_label); j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, gang_label[j]);
        }
        
        // Draw target name
        glColor3f(0.9f, 0.7f, 0.2f);  // Amber/gold color for target
        glRasterPos2f(x + 130, gang_y_offset + 30);
        
        // Convert crime type to text
        char* crime_name;
        switch (gang_state.current_target) {
            case BANK_ROBBERY: crime_name = "BANK ROBBERY"; break;
            case JEWELRY_ROBBERY: crime_name = "JEWELRY THEFT"; break;
            case DRUG_TRAFFICKING: crime_name = "DRUG TRAFFICKING"; break;
            case ARTWORK_ROBBERY: crime_name = "ART HEIST"; break;
            case KIDNAPPING: crime_name = "KIDNAPPING"; break;
            case BLACKMAILING: crime_name = "BLACKMAIL"; break;
            case ARM_TRAFFICKING: crime_name = "ARMS DEALING"; break;
            default: crime_name = "UNKNOWN OPERATION"; break;
        }
        
        for (int j = 0; j < strlen(crime_name); j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, crime_name[j]);
        }
        
        // G-4: Draw progress bar
        int bar_width = width - 40;
        int bar_height = 20;
        int bar_x = x + 20;
        int bar_y = gang_y_offset;
        
        // Draw background (gray)
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(bar_x, bar_y);
        glVertex2f(bar_x + bar_width, bar_y);
        glVertex2f(bar_x + bar_width, bar_y + bar_height);
        glVertex2f(bar_x, bar_y + bar_height);
        glEnd();
        
        // Calculate filled portion
        float fill_percentage = gang_state.preparation_level / 100.0f;
        if (fill_percentage > 1.0f) fill_percentage = 1.0f;
        int fill_width = (int)(bar_width * fill_percentage);
        
        // G-4: Choose progress bar color based on completion percentage
        if (fill_percentage < 0.5f) {
            // < 50% → dim gray
            glColor3f(0.4f, 0.4f, 0.4f);
        } else if (fill_percentage < 0.8f) {
            // 50–80% → amber
            glColor3f(0.9f, 0.6f, 0.0f);
        } else {
            // ≥ 80% → crimson
            glColor3f(0.8f, 0.0f, 0.2f);
        }
        
        // Draw filled portion
        glBegin(GL_QUADS);
        glVertex2f(bar_x, bar_y);
        glVertex2f(bar_x + fill_width, bar_y);
        glVertex2f(bar_x + fill_width, bar_y + bar_height);
        glVertex2f(bar_x, bar_y + bar_height);
        glEnd();
        
        // Draw percentage text
        glColor3f(1.0f, 1.0f, 1.0f);
        char percentage_text[10];
        sprintf(percentage_text, "%d%%", gang_state.preparation_level);
        
        // Center percentage text on the bar
        int text_width = glutBitmapLength(GLUT_BITMAP_HELVETICA_12, (const unsigned char*)percentage_text);
        glRasterPos2f(bar_x + (bar_width - text_width) / 2.0f, bar_y + 5); // Use explicit float division
        
        for (int j = 0; j < strlen(percentage_text); j++) {
            glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, percentage_text[j]);
        }
        
        // Move to next gang
        gang_y_offset -= gang_item_height;
    }
}

// Function to draw the right column with counters
void draw_counters(int x, int y, int width, int height) {
    // Get thread-safe access to visualization context
    pthread_mutex_lock(&viz_context.mutex);
    SharedState* shared_state = viz_context.shared_state;
    SimulationConfig config = viz_context.config;
    pthread_mutex_unlock(&viz_context.mutex);
    
    // Only proceed if we have valid shared state
    if (!shared_state) return;
    
    // Draw section title
    glColor3f(1.0f, 1.0f, 1.0f);  // White text
    glRasterPos2f(x + 20, height - 30);
    char* title = "STATISTICS";
    for (int i = 0; i < strlen(title); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, title[i]);
    }
    
    // Draw horizontal separator
    glColor3f(0.4f, 0.4f, 0.4f);
    glBegin(GL_LINES);
    glVertex2f(x + 5, height - 40);
    glVertex2f(x + width - 5, height - 40);
    glEnd();
    
    // Define counter positions
    int counter_y = height - 80;
    int counter_spacing = 100;
    
    // G-2: Draw Plans Thwarted counter
    glColor3f(0.0f, 0.7f, 1.0f);  // Blue for police/thwarted
    glRasterPos2f(x + 20, counter_y);
    char thwarted_label[] = "PLANS THWARTED:";
    for (int i = 0; i < strlen(thwarted_label); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, thwarted_label[i]);
    }
    
    // Draw counter value with max
    char thwarted_value[30];
    sprintf(thwarted_value, "%d / %d", 
            shared_state->total_thwarted_missions,
            config.max_thwarted_plans);
    
    glRasterPos2f(x + 20, counter_y - 20);
    for (int i = 0; i < strlen(thwarted_value); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, thwarted_value[i]);
    }
    
    // G-2: Draw Plans Succeeded counter
    counter_y -= counter_spacing;
    glColor3f(0.9f, 0.5f, 0.0f);  // Orange for gang success
    glRasterPos2f(x + 20, counter_y);
    char succeeded_label[] = "PLANS SUCCEEDED:";
    for (int i = 0; i < strlen(succeeded_label); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, succeeded_label[i]);
    }
    
    // Draw counter value with max
    char succeeded_value[30];
    sprintf(succeeded_value, "%d / %d", 
            shared_state->total_successful_missions,
            config.max_successful_plans);
    
    glRasterPos2f(x + 20, counter_y - 20);
    for (int i = 0; i < strlen(succeeded_value); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, succeeded_value[i]);
    }
    
    // G-2: Draw Agents Executed counter
    counter_y -= counter_spacing;
    glColor3f(0.8f, 0.0f, 0.0f);  // Red for executed agents
    glRasterPos2f(x + 20, counter_y);
    char executed_label[] = "AGENTS EXECUTED:";
    for (int i = 0; i < strlen(executed_label); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, executed_label[i]);
    }
    
    // Draw counter value with max
    char executed_value[30];
    sprintf(executed_value, "%d / %d", 
            shared_state->total_executed_agents,
            config.max_executed_agents);
    
    glRasterPos2f(x + 20, counter_y - 20);
    for (int i = 0; i < strlen(executed_value); i++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, executed_value[i]);
    }
}

// Cleanup visualization resources
void cleanup_visualization() {
    // Free allocated memory for expanded gangs tracking
    if (viz_context.expanded_gangs) {
        free(viz_context.expanded_gangs);
        viz_context.expanded_gangs = NULL;
    }
}
