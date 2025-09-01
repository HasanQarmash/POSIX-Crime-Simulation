#!/bin/bash
# This script builds and runs the fixed version of the crime simulation

echo "Building fixed version of crime simulation..."
cd "$(dirname "$0")"
make clean

# Compile the project
mkdir -p build
gcc -Wall -g -pthread -Iinclude -c src/config.c -o build/config.o
gcc -Wall -g -pthread -Iinclude -c src/gang.c -o build/gang.o
gcc -Wall -g -pthread -Iinclude -c src/ipc.c -o build/ipc.o
gcc -Wall -g -pthread -Iinclude -c src/main.c -o build/main.o
gcc -Wall -g -pthread -Iinclude -c src/police.c -o build/police.o
gcc -Wall -g -pthread -Iinclude -c src/utils.c -o build/utils.o
gcc -Wall -g -pthread -Iinclude -c src/thread_safe_drawing.c -o build/thread_safe_drawing.o
gcc -Wall -g -pthread -Iinclude -c src/visualization.c -o build/visualization.o

# Link everything
gcc -o build/crime_sim build/*.o -lGL -lGLU -lglut -lm

echo "Running crime simulation..."

# Set up X11 display for WSL2
if grep -q "WSL2" /proc/version; then
  echo "WSL2 detected, setting up X11 display"
  export DISPLAY=$(grep -m 1 nameserver /etc/resolv.conf | awk '{print $2}'):0
  export LIBGL_ALWAYS_INDIRECT=1
  echo "Using DISPLAY=$DISPLAY"
fi

# Run the simulation
./build/crime_sim config/simulation_config.txt

echo "Done!"
