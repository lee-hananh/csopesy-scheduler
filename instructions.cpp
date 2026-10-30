#include "emulator.h"

// TODO: build the process's instruction list (PRINT, DECLARE, ADD,
// SUBTRACT, SLEEP, FOR). Called once when a process is created.
// Set p.total to the number of instructions.
void generateInstructions(Process& p) {
    p.total = 100;
}

// TODO: execute the next instruction of p on the given core.
// Called by a CPU worker with mtx already locked. PRINT output goes into p.logs
// as "(timestamp) Core:N "msg"". Must advance p.done.
void execInstruction(Process& p, int core) {
    (void)core;
    p.done++;
}
