# 🕵️ CrimeSim: Advanced Multi-Process Crime Fighting Simulation

<div align="center">

![Crime Simulation](https://img.shields.io/badge/Simulation-Crime%20Fighting-red?style=for-the-badge&logo=shield)
![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![OpenGL](https://img.shields.io/badge/OpenGL-5586A4?style=for-the-badge&logo=opengl&logoColor=white)
![POSIX](https://img.shields.io/badge/POSIX-Threads-blue?style=for-the-badge&logo=linux&logoColor=white)
![License](https://img.shields.io/badge/License-MIT-green?style=for-the-badge)

**A sophisticated real-time simulation of organized crime vs. law enforcement with advanced multi-processing, multi-threading, and OpenGL visualization**

[Features](#-features) • [Installation](#-installation) • [Usage](#-usage) • [Configuration](#-configuration) • [Architecture](#-architecture) • [Contributing](#-contributing)

</div>

## 🌟 Overview

CrimeSim is an advanced multi-process and multi-threading application that simulates the complex dynamics between organized crime gangs and law enforcement agencies. The simulation features infiltrated secret agents, sophisticated crime planning, police intelligence operations, and stunning real-time OpenGL visualization.

### 🎯 Key Highlights

- **Real-time Multi-Process Architecture**: Separate processes for gangs and police with advanced IPC
- **Intelligent Agent System**: Secret agents infiltrate gangs and report to police headquarters
- **Dynamic Crime Planning**: Gangs plan and execute various types of criminal activities
- **Advanced Visualization**: Real-time OpenGL graphics with interactive dashboards
- **Realistic Simulation**: Complex hierarchies, preparation phases, and success/failure mechanics
- **Cross-Platform Support**: Linux, macOS, and Windows (WSL) compatibility

## 🚀 Features

### 🏛️ Core Simulation
- **Multiple Crime Types**: Bank robbery, jewelry theft, drug trafficking, kidnapping, art theft, blackmail, arms trafficking
- **Gang Hierarchies**: 7-level ranking system with promotion mechanics
- **Secret Agent Operations**: Undercover agents collect intelligence and face discovery risks
- **Police Intelligence**: Advanced decision-making based on multiple intelligence reports
- **Prison System**: Arrest mechanics with configurable sentence lengths

### 🎨 Advanced Visualization
- **Real-time OpenGL Graphics**: Smooth 60 FPS visualization with hardware acceleration
- **Interactive Dashboard**: Gang status, mission progress, and statistics
- **Mission Animations**: Visual representation of preparation phases and outcomes
- **Text & Graphical Modes**: Automatic fallback to text mode when no display available
- **Dynamic Statistics**: Live counters for missions, arrests, and agent casualties

### ⚙️ Technical Features
- **Multi-Processing**: Fork-based process creation for gang and police entities
- **Multi-Threading**: POSIX threads for gang members and police operations
- **Advanced IPC**: Message queues, shared memory, and semaphores for synchronization
- **Thread-Safe Operations**: Mutex protection for all shared resources
- **Memory Management**: Proper cleanup and leak prevention
- **Signal Handling**: Graceful shutdown and resource cleanup

## 📋 Requirements

### System Requirements
- **OS**: Linux, macOS, or Windows with WSL
- **Compiler**: GCC with C99 support
- **Libraries**: POSIX threads, OpenGL/GLUT
- **Memory**: Minimum 512MB RAM
- **Graphics**: OpenGL 1.1+ compatible graphics card (optional)

### Dependencies
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential libgl1-mesa-dev freeglut3-dev

# CentOS/RHEL/Fedora
sudo yum install gcc make mesa-libGL-devel freeglut-devel
# or
sudo dnf install gcc make mesa-libGL-devel freeglut-devel

# macOS (with Homebrew)
brew install gcc freeglut
```

## 🛠️ Installation

### Quick Start
```bash
# Clone the repository
git clone https://github.com/HasanQarmash/POSIX-Crime-Simulation.git
cd POSIX-Crime-Simulation

# Build the project
make

# Run with default configuration
make run
```

### Custom Build Options
```bash
# Debug build with symbols
make debug

# Clean build directory
make clean

# Run with custom configuration
./build/crime_sim config/custom_config.txt
```

## 🎮 Usage

### Basic Operation
```bash
# Standard simulation
./build/crime_sim config/simulation_config.txt

# Text-only mode (no graphics)
DISPLAY= ./build/crime_sim config/simulation_config.txt
```

### Visualization Controls
- **Mouse**: Click gang panels to expand/collapse member details
- **Keyboard**: 
  - `q` or `ESC`: Quit simulation
  - `p`: Pause/Resume simulation
  - `r`: Reset statistics
  - Arrow keys: Scroll through gang lists

### Debugging
```bash
# Debug with GDB
make debug
gdb ./build/crime_sim

# Memory leak detection
valgrind --leak-check=full ./build/crime_sim config/simulation_config.txt
```

## ⚙️ Configuration

The simulation behavior is controlled through `config/simulation_config.txt`:

### Gang Configuration
```ini
MIN_GANGS=2                    # Minimum number of gangs
MAX_GANGS=4                    # Maximum number of gangs
MIN_MEMBERS_PER_GANG=2         # Minimum gang size
MAX_MEMBERS_PER_GANG=3         # Maximum gang size
GANG_RANKS=7                   # Number of hierarchy levels
```

### Crime Planning
```ini
PREPARATION_TIME_MIN=5         # Minimum preparation time
PREPARATION_TIME_MAX=10        # Maximum preparation time
MIN_PREPARATION_LEVEL=30       # Minimum prep level required
MAX_PREPARATION_LEVEL=90       # Maximum prep level achievable
FALSE_INFO_PROBABILITY=30      # Chance of spreading disinformation
```

### Secret Agents
```ini
AGENT_INFILTRATION_SUCCESS_RATE=30  # Agent placement probability
AGENT_SUSPICION_THRESHOLD=85        # Discovery risk threshold
POLICE_ACTION_THRESHOLD=80          # Action decision threshold
```

### Termination Conditions
```ini
MAX_THWARTED_PLANS=100         # Max police successes
MAX_SUCCESSFUL_PLANS=100       # Max gang successes
MAX_EXECUTED_AGENTS=100        # Max agent casualties
```

## 🏗️ Architecture

### Process Structure
```
Main Process
├── Gang Process 1
│   ├── Gang Member Thread 1
│   ├── Gang Member Thread 2
│   └── Gang Member Thread N
├── Gang Process 2
│   └── ...
├── Police Process
│   └── Police Thread
└── Visualization Thread
```

### Inter-Process Communication
- **Message Queues**: Intelligence reports from agents to police
- **Shared Memory**: Global simulation state and statistics
- **Semaphores**: Synchronization of shared resources
- **Signal Handling**: Graceful termination and cleanup

### Core Components

#### Gang Module (`src/gang.c`)
- Gang initialization and member management
- Crime planning and execution logic
- Internal investigation for agent discovery
- Multi-threaded member behavior simulation

#### Police Module (`src/police.c`)
- Intelligence report processing
- Decision-making algorithms
- Arrest coordination with gangs
- Agent management and protection

#### Visualization Module (`src/visualization.c`)
- OpenGL-based real-time graphics
- Interactive dashboard components
- Animation system for mission progress
- Automatic text-mode fallback

#### IPC Module (`src/ipc.c`)
- Message queue management
- Shared memory operations
- Semaphore synchronization
- Cross-process communication protocols

## 📊 Simulation Flow

1. **Initialization**: Gangs and police processes are created with configured parameters
2. **Mission Planning**: Gang leaders select crime types and set preparation requirements
3. **Preparation Phase**: Gang members work to reach required preparation levels
4. **Intelligence Gathering**: Secret agents collect information and report to police
5. **Police Analysis**: Intelligence reports are processed and action decisions made
6. **Mission Execution**: Gangs attempt crimes while police may intervene
7. **Consequences**: Successful missions, arrests, or agent discoveries occur
8. **Cycle Repeat**: New missions are planned and the cycle continues

## 📈 Performance Features

- **Optimized Rendering**: Efficient OpenGL calls with display lists
- **Memory Pool Management**: Pre-allocated structures for better performance
- **Selective Updates**: Only redraw changed elements
- **Background Processing**: Non-blocking I/O for smooth visualization
- **Resource Monitoring**: Built-in memory and thread health checking

## 🧪 Testing

### Unit Testing
```bash
# Test individual components
make test_gang
make test_police
make test_ipc
```

### Integration Testing
```bash
# Test full simulation with mock data
make test_integration

# Stress test with maximum configuration
make stress_test
```

## 🐛 Troubleshooting

### Common Issues

**Graphics not working?**
```bash
# Check OpenGL support
glxinfo | grep "direct rendering"

# Force software rendering
export LIBGL_ALWAYS_SOFTWARE=1
./build/crime_sim config/simulation_config.txt
```

**Compilation errors?**
```bash
# Install missing dependencies
sudo apt-get install build-essential libgl1-mesa-dev freeglut3-dev

# Check GCC version
gcc --version  # Should be 4.9+
```

**Simulation hangs?**
```bash
# Enable debug mode
make debug
gdb ./build/crime_sim
```

## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details.

### Development Setup
```bash
# Fork and clone the repository
git clone https://github.com/HasanQarmash/POSIX-Crime-Simulation.git

# Create a feature branch
git checkout -b feature/amazing-feature

# Make your changes and test
make debug
./build/crime_sim config/simulation_config.txt

# Submit a pull request
```

### Code Style
- Follow K&R C style conventions
- Use meaningful variable names
- Comment complex algorithms
- Include unit tests for new features

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Dr. Hanna Bullata** - Project supervisor and guidance
- **Birzeit University** - Faculty of Engineering and Technology
- **OpenGL Community** - Graphics programming resources
- **POSIX Standards** - Multi-threading and IPC specifications

## 📞 Contact

- **Author**: Hasan Qarmash
- **Email**: your.email@example.com
- **Project Link**: [https://github.com/HasanQarmash/POSIX-Crime-Simulation](https://github.com/HasanQarmash/POSIX-Crime-Simulation)

---

<div align="center">

**⭐ Star this repository if you found it helpful! ⭐**

Made with ❤️ for educational purposes and crime simulation research

</div>
