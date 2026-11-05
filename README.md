# CSOPESY Major Output 1 - Process Scheduler and CLI

**Course:** CSOPESY
**Section:** S01
**Group:** Guillermo, Lee, Lim

**Members**

- Iain Guillermo
- Hannah Lee
- Jenrick Lim

**GitHub:** https://github.com/lee-hananh/csopesy-scheduler

## Entry File

`main.cpp`

The `main()` function is located in this file.

## About

A command-line interpreter (CLI) and multi-core process scheduler emulator built in C++. The system simulates an operating system's CPU scheduler and process multiplexer (inspired by Linux `screen`), supporting concurrent process execution across configurable worker threads, randomized instruction generation, memory variable storage, and real-time utilization logging.

## Requirements

- Windows 10 or 11 (64-bit)
- Visual Studio 2022 (17.10 or newer, needed for `.slnx` files) with the **Desktop development with C++** workload installed
- GLFW and Dear ImGui are already included in the repository, so no extra installation is needed

## How to Run

1. Clone or download the repository.
2. Install the **C/C++** extension in VS Code.
3. Press `Ctrl+Shift+B` to trigger the build task, or use `F5` to start debugging/running.
4. Ensure the active workspace run directory points to the project root where `config.txt` resides.
5. Run `initialize` first to load configuration parameters from `config.txt` and launch worker threads.
6. Type `scheduler-start` to start automatic process generation or use `screen -s <name>` to manually spawn a process screen.
7. Use `screen -ls` to view running/finished processes and current CPU utilization.
8. Type `report-util` to export the current status log to `csopesy-log.txt`.
9. Enter `exit` at the main console prompt to shut down all worker threads and exit the emulator.

## Features

### Main Menu Console

Commands available at the `root:\>` main prompt:

| Command | Action | 
| ----- | ----- | 
| `initialize` | Reads `config.txt`, initializes core worker threads, and enables scheduler operations | 
| `screen -s <name>` | Creates a new process with the specified name and enters its interactive screen | 
| `screen -r <name>` | Attaches to an active, non-finished process screen | 
| `screen -ls` | Displays live CPU utilization, used/available cores, and process tables | 
| `scheduler-start` | Starts generating dummy processes at the configured frequency | 
| `scheduler-stop` | Pauses automatic dummy process generation | 
| `report-util` | Generates and exports CPU utilization and process logs to `csopesy-log.txt` | 
| `exit` | Safely stops worker threads and terminates the CLI emulator | 

### Screen Multiplexer

When attached to a process screen (`screen -s` or `screen -r`), the user can run:

| Command | Action | 
| ----- | ----- | 
| `process-smi` | Displays process ID, execution logs, current instruction line, and status | 
| `exit` | Detaches from the current process screen and returns to the main CLI menu | 

### Simulated Process Instructions

Processes consist of randomly generated sequences of instructions:

* **`PRINT`**: Outputs text or variable values to process logs with execution timestamp and assigned CPU core.

* **`DECLARE`**: Initializes a process memory variable (`uint16_t`) with a default value.

* **`ADD` / `SUBTRACT`**: Performs arithmetic operations on variables/literals clamped between `[0, 65535]`.

* **`SLEEP`**: Pauses process execution for a specified number of CPU ticks and relinquishes the core.

* **`FOR_LOOP`**: Executes nested blocks of instructions up to a maximum nesting depth of 3.

## Project Structure

```
.vscode/             VS Code workspace configuration (launch.json, tasks.json)
config.txt           System configuration file (num-cpu, frequencies, instruction limits)
emulator.h           Shared header containing structs, enums, global config, and prototypes
main.cpp             Entry point, CLI command handler, CPU worker loop, sleeper & generator threads
config.cpp           Parser for config.txt parameter initialization
instructions.cpp     Random instruction stream generator and execution logic
report.cpp           Report exporter outputting system state to csopesy-log.txt
csopesy-log.txt      Generated log output file from report-util
```