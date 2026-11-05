#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "emulator.h"

using namespace std;

// one cpu tick (in ms)
const int TICK_MS = 50;

using ProcPtr = shared_ptr<Process>;

Config cfg;
mutex mtx;
bool initialized = false;
condition_variable cv;
queue<ProcPtr> readyQueue;
vector<ProcPtr> allProcs;
vector<bool> coreBusy;              // busy flag for each core
vector<thread> workers;
thread generator;                   // dummy process generator
thread sleeper;                     // counts down sleep ticks
vector<ProcPtr> sleepers;           // processes currently blocked on sleep
atomic<bool> stopAll{false};        // signals all threads to terminate
atomic<bool> generating{false};     // true while scheduler-start is active
int nextId = 1;

// returns current local time (MM/DD/YYYY HH:MM:SSAM/PM)
string now() {
    time_t t = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%m/%d/%Y %I:%M:%S%p", localtime(&t));
    return buf;
}

int randomInt(int lo, int hi) {
    static thread_local mt19937 rng{random_device{}()};
    return uniform_int_distribution<int>(lo, hi)(rng);
}

void clearScreen() {
// #ifdef _WIN32
//     system("cls");
// #else
//     cout << "\033[2J\033[H";
// #endif
    cout << string(50, '\n');
}

ProcPtr findProcess(const string& name) {
    lock_guard<mutex> lk(mtx);
    for (auto& p : allProcs)
        if (p->name == name) return p;
    return nullptr;
}

ProcPtr makeProcess(const string& name) {
    auto p = make_shared<Process>();
    p->name = name;
    generateInstructions(*p);
    p->startTime = now();
    lock_guard<mutex> lk(mtx);
    p->id = nextId++;
    allProcs.push_back(p);
    readyQueue.push(p);
    cv.notify_one();
    return p;
}

// the cpu core thread
// waits for process in ready queue then executes one instruction per tick
void workerLoop(int core) {
    while (true) {
        ProcPtr p;
        {
            unique_lock<mutex> lk(mtx);
            cv.wait(lk, [] { return stopAll || !readyQueue.empty(); });
            if (stopAll) return;
            p = readyQueue.front();
            readyQueue.pop();
            p->core = core;
            coreBusy[core] = true;
        }
        while (!stopAll) {
            this_thread::sleep_for(chrono::milliseconds(TICK_MS * (cfg.delayPerExec + 1)));
            lock_guard<mutex> lk(mtx);
            execInstruction(*p, core);
            if (p->done >= p->total) break;
            if (p->sleepTicks > 0) break;  // sleep -> relinquish cpu
        }
        lock_guard<mutex> lk(mtx);
        if (p->done >= p->total) {
            p->finished = true;
            p->endTime = now();
        } else if (p->sleepTicks > 0) {
            sleepers.push_back(p);
        }
        p->core = -1;
        coreBusy[core] = false;
    }
}

// sleep manager
// decrements remaining sleep ticks every tick then returns process back to ready queue when it reaches 0
void sleeperLoop() {
    while (!stopAll) {
        this_thread::sleep_for(chrono::milliseconds(TICK_MS));
        lock_guard<mutex> lk(mtx);
        for (auto it = sleepers.begin(); it != sleepers.end();) {
            if (--(*it)->sleepTicks <= 0) {
                (*it)->sleepTicks = 0;
                readyQueue.push(*it);
                cv.notify_one();
                it = sleepers.erase(it);
            } else {
                ++it;
            }
        }
    } 
}

// process generator
void generatorLoop() {
    while (!stopAll) {
        for (int t = 0; t < cfg.batchFreq && !stopAll; t++)
            this_thread::sleep_for(chrono::milliseconds(TICK_MS));
        if (stopAll) return;
        if (!generating) continue;
        int id;
        string name;
        do {
            {
                lock_guard<mutex> lk(mtx);
                id = nextId;
            }
            ostringstream n;
            n << "p" << setw(2) << setfill('0') << id;
            name = n.str();
            if (findProcess(name)) {
                lock_guard<mutex> lk(mtx);
                nextId++;
            }
        } while (findProcess(name));
        makeProcess(name);
    }
}

// builds cpu utilizization report
string buildReport() {
    lock_guard<mutex> lk(mtx);
    int busy = 0;
    for (bool b : coreBusy) busy += b;
    int util = cfg.numCpu ? busy * 100 / cfg.numCpu : 0;
    ostringstream out;
    out << "CPU utilization: " << util << "%\n";
    out << "Cores used: " << busy << "\n";
    out << "Cores available: " << cfg.numCpu - busy << "\n\n";
    out << "--------------------------------------\n";
    out << "Running processes:\n";
    for (auto& p : allProcs)
        if (!p->finished && p->core >= 0)
            out << left << setw(10) << p->name << " (" << p->startTime << ")  Core: " << p->core
                << "  " << p->done << " / " << p->total << "\n";
    out << "\nFinished processes:\n";
    for (auto& p : allProcs)
        if (p->finished)
            out << left << setw(10) << p->name << " (" << p->endTime << ")  Finished  " << p->done
                << " / " << p->total << "\n";
    out << "--------------------------------------\n";
    return out.str();
}

// prints process name, id, print logs, finished or current instruction line, and total lines of code
void processSmi(const ProcPtr& p) {
    lock_guard<mutex> lk(mtx);
    cout << "Process name: " << p->name << "\n";
    cout << "ID: " << p->id << "\n";
    cout << "Logs:\n";
    for (auto& l : p->logs) cout << l << "\n";
    cout << "\n";
    if (p->finished) {
        cout << "Finished!\n";
    } else {
        cout << "Current instruction line: " << p->done << "\n";
        cout << "Lines of code: " << p->total << "\n";
    }
}

// process screen loop
void enterScreen(const ProcPtr& p) {
    clearScreen();
    processSmi(p);
    string line;
    while (true) {
        cout << "\nroot:\\> ";
        if (!getline(cin, line)) return;
        istringstream iss(line);
        string cmd;
        iss >> cmd;
        if (cmd.empty()) continue;
        if (cmd == "exit") break;
        if (cmd == "process-smi") processSmi(p);
        else cout << "Unknown command: " << cmd << "\n";
    }
    clearScreen();
}

// loads config.txt and starts generator and sleeper threads
void initialize() {
    loadConfig();
    coreBusy.assign(cfg.numCpu, false);
    for (int i = 0; i < cfg.numCpu; i++) workers.emplace_back(workerLoop, i);
    generator = thread(generatorLoop);
    sleeper = thread(sleeperLoop);
    initialized = true;
}

// stops and joins all worker, generator, and sleeper threads
void shutdown() {
    stopAll = true;
    cv.notify_all();
    for (auto& w : workers) w.join();
    if (generator.joinable()) generator.join();
    if (sleeper.joinable()) sleeper.join();
}

void printBanner() {
    cout << R"ART(
  ___ ___  ___  ___ ___ ___ __   __
 / __/ __|/ _ \| _ \ __/ __\ \ / /
| (__\__ \ (_) |  _/ _|\__ \\ V /
 \___|___/\___/|_| |___|___/ |_|
)ART";
    cout << "----------------------------------------\n";
    cout << "Welcome to CSOPESY Emulator!\n\n";
    cout << "Developers:\nGuillermo, Lee, Lim\n\n";
    cout << "Last updated: 10-31-2026\n";
    cout << "----------------------------------------\n";
}

// handles the screen commands
void handleScreen(const vector<string>& args) {
    if (args.size() >= 3 && args[1] == "-s") {
        if (findProcess(args[2])) {
            cout << "Process " << args[2] << " already exists.\n";
            return;
        }
        enterScreen(makeProcess(args[2]));
        printBanner();
    } else if (args.size() >= 3 && args[1] == "-r") {
        auto p = findProcess(args[2]);
        bool done;
        {
            lock_guard<mutex> lk(mtx);
            done = p && p->finished;
        }
        if (!p || done) {
            cout << "Process " << args[2] << " not found.\n";
            return;
        }
        enterScreen(p);
        printBanner();
    } else if (args.size() == 2 && args[1] == "-ls") {
        cout << buildReport();
    } else {
        cout << "Usage: screen -s <name> | screen -r <name> | screen -ls\n";
    }
}

int main() {
    printBanner();
    string line;
    while (true) {
        cout << "\nroot:\\> ";
        if (!getline(cin, line)) break;
        istringstream iss(line);
        vector<string> args;
        string tok;
        while (iss >> tok) args.push_back(tok);
        if (args.empty()) continue;
        const string& cmd = args[0];

        if (cmd == "exit") break;
        if (cmd == "initialize") {
            if (initialized) cout << "Already initialized.\n";
            else {
                initialize();
                cout << "Initialized with " << cfg.numCpu << " CPU cores.\n";
            }
            continue;
        }
        if (!initialized) {
            cout << "Run 'initialize' first.\n";
            continue;
        }
        if (cmd == "screen") handleScreen(args);
        else if (cmd == "scheduler-start") {
            generating = true;
            cout << "Generating dummy processes...\n";
        } else if (cmd == "scheduler-stop") {
            generating = false;
            cout << "Stopped generating dummy processes.\n";
        } else if (cmd == "report-util") handleReportUtil();
        else cout << "Unknown command: " << cmd << "\n";
    }
    shutdown();
    return 0;
}