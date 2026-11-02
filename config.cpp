#include "emulator.h"
#include <fstream>
#include <iostream>
#include <sstream>

// TODO: read config.txt and fill in cfg.
// Called once by "initialize". Add any new fields to Config in emulator.h.
void loadConfig() {
    std::ifstream file("config.txt");
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open config.txt. Using default configuration.\n";
        return;
    }

    std::string key;
    while (file >> key) {
        if (key == "num-cpu") {
            file >> cfg.numCpu;
        } else if (key == "batch-process-freq") {
            file >> cfg.batchFreq;
        } else if (key == "min-ins") {
            file >> cfg.minIns;
        } else if (key == "max-ins") {
            file >> cfg.maxIns;
        } else if (key == "delay-per-exec") {
            file >> cfg.delayPerExec;
        } else {
            std::string dummy;
            file >> dummy;
        }
    }
    file.close();
}