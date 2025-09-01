CC = gcc
CFLAGS = -Wall -g -pthread
LDFLAGS = -lGL -lGLU -lglut -lm

# Source and object directories
SRC_DIR = src
INC_DIR = include
BUILD_DIR = build

# Source files
SRCS = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRCS))

# Make sure main.c is replaced with main_fixed.c for compilation
main_fixed: SRCS := $(filter-out $(SRC_DIR)/main.c,$(SRCS)) $(SRC_DIR)/main_fixed.c
main_fixed: OBJS := $(filter-out $(BUILD_DIR)/main.o,$(OBJS)) $(BUILD_DIR)/main_fixed.o
main_fixed: all

# Executable name
TARGET = $(BUILD_DIR)/crime_sim

# Main target
all: $(BUILD_DIR) $(TARGET)

# Create build directory if it doesn't exist
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# Link object files to create executable
$(TARGET): $(OBJS)
	$(CC) -o $@ $^ $(LDFLAGS)

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

# Run the program with the default configuration
run: $(TARGET)
	./$(TARGET) config/simulation_config.txt

# Run the fixed version
run_fixed: main_fixed
	./$(TARGET) config/simulation_config.txt

# Clean build files
clean:
	rm -rf $(BUILD_DIR)/*

# Debug helpers
debug: CFLAGS += -DDEBUG
debug: all

.PHONY: all run clean debug
