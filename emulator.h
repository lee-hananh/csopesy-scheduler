#pragma once

#include <mutex>
#include <string>
#include <vector>

struct Config {
    int numCpu = 4;
    int batchFreq = 20;
    int minIns = 100;
    int maxIns = 200;
    int delayPerExec = 0;
};

struct Process {
    std::string name;
    int id = 0;
    int total = 0;
    int done = 0;
    int core = -1;
    bool finished = false;
    std::string startTime;
    std::string endTime;
    std::vector<std::string> logs;
};

extern Config cfg;
extern std::mutex mtx;

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
