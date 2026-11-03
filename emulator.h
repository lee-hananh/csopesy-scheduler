#pragma once

#include <mutex>
#include <string>
#include <vector>
#include <map>
#include <cstdint>

// process instructions
enum class OpCode { PRINT, DECLARE, ADD, SUBTRACT, SLEEP, FOR_LOOP };

// a single process instruction
struct Instruction {
    OpCode op;
    std::string var1;               // target variable / print variable name
    std::string var2;               // left operand variable for add/subtract
    std::string var3;               // right operand variable for add/subtract
    uint16_t val1 = 0;              // declare value / sleep ticks
    uint16_t val2 = 0;              // left operand literal for add/subtract
    uint16_t val3 = 0;              // right operand literal for add/subtract
    bool isVal2 = false;            // if true, use val2 literal, else use var2 variable
    bool isVal3 = false;            // if true, use val3 literal, else use var3 variable
    int repeats = 1;                // Ffor loop iteration count 
    std::vector<Instruction> body;  // for loop body, nestable (max depth = 3)
    std::string msg;                // print message text
};

// schedule params loaded from config.txt
struct Config {
    int numCpu = 4;
    int batchFreq = 20;
    int minIns = 100;
    int maxIns = 200;
    int delayPerExec = 0;
};

// process control block
struct Process {
    std::string name;
    int id = 0;
    int total = 0;
    int done = 0;
    int core = -1;              // current core running the process
    int sleepTicks = 0;
    bool finished = false;
    std::string startTime;
    std::string endTime;
    std::vector<std::string> logs;                  // print output
    std::map<std::string, uint16_t> variables;      // process memory
    std::vector<Instruction> instructions;          // instruction stream to execute
};

extern Config cfg;
extern std::mutex mtx;

// main.cpp
std::string now();
int randomInt(int lo, int hi);
std::string buildReport();

// config.cpp
void loadConfig();

// instructions.cpp
void generateInstructions(Process& p);
void execInstruction(Process& p, int core);

// report.cpp
void handleReportUtil();