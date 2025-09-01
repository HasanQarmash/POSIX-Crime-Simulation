#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include "config.h"

// Function prototypes
int random_int(int min, int max);
double random_double(double min, double max);
bool random_event(int probability_percentage);
void delay_ms(int milliseconds);
void log_message(const char* format, ...);
const char* crime_type_to_string(CrimeType type);

#endif /* UTILS_H */
