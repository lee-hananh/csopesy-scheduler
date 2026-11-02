#include <iostream>
#include <fstream>
#include "emulator.h"

// TODO: handle "report-util" by saving the utilization report to csopesy-log.txt.
// buildReport() (used by "screen -ls") returns the report text.
void handleReportUtil() {
    std::string reportData = buildReport();
    
    std::ofstream outFile("csopesy-log.txt");
    if (outFile.is_open()) {
        outFile << reportData;
        outFile.close();
        std::cout << "Report generated at csopesy-log.txt!\n";
    } else {
        std::cout << "Error: Unable to create report file csopesy-log.txt.\n";
    }
}
