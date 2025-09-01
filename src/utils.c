#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stdbool.h>
#include "../include/utils.h"
#include "../include/config.h"

// Generate a random integer between min and max (inclusive)
int random_int(int min, int max) {
    return min + rand() % (max - min + 1);
}

// Generate a random double between min and max
double random_double(double min, double max) {
    return min + (max - min) * ((double)rand() / RAND_MAX);
}

// Determine if an event occurs with the given probability percentage
bool random_event(int probability_percentage) {
    return (rand() % 100) < probability_percentage;
}

// Delay execution for the specified number of milliseconds
void delay_ms(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

// Log a message with timestamp
void log_message(const char* format, ...) {
    // Get current time
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // Print timestamp
    printf("[%s] ", time_str);
    
    // Print message with variable arguments
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    
    // Add newline
    printf("\n");
}

// Convert crime type to string
const char* crime_type_to_string(CrimeType type) {
    switch (type) {
        case BANK_ROBBERY:
            return "Bank Robbery";
        case JEWELRY_ROBBERY:
            return "Jewelry Robbery";
        case DRUG_TRAFFICKING:
            return "Drug Trafficking";
        case ARTWORK_ROBBERY:
            return "Artwork Robbery";
        case KIDNAPPING:
            return "Kidnapping";
        case BLACKMAILING:
            return "Blackmailing";
        case ARM_TRAFFICKING:
            return "Arm Trafficking";
        default:
            return "Unknown Crime";
    }
}
