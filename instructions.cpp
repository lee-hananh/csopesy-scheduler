#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include "emulator.h"

static Instruction createRandomInstruction() {
    Instruction inst;
    
    int type = randomInt(0, 1);

    if (type == 0) {
        // PRINT 
        inst.op = OpCode::PRINT;
        
        if (randomInt(0, 1) == 0) {
            inst.msg = "Hello world from ";
            inst.var1 = ""; 
        } else {
            inst.msg = "Value from var: ";
            inst.var1 = "var";
        }
    } else {
        // DECLARE 
        inst.op = OpCode::DECLARE;
        inst.var1 = "var"; 
        inst.val1 = static_cast<uint16_t>(randomInt(1, 100)); 
    }

    return inst;
}

// TODO: build the process's instruction list (PRINT, DECLARE, ADD,
// SUBTRACT, SLEEP, FOR). Called once when a process is created.
// Set p.total to the number of instructions.
void generateInstructions(Process& p) {
    p.instructions.clear();
    p.variables.clear();
    
    int count = randomInt(cfg.minIns, cfg.maxIns);
    for (int i = 0; i < count; ++i) {
        p.instructions.push_back(createRandomInstruction());
    }
    
    p.total = static_cast<int>(p.instructions.size());
    p.done = 0;
}

// TODO: execute the next instruction of p on the given core.
// Called by a CPU worker with mtx already locked. PRINT output goes into p.logs
// as "(timestamp) Core:N "msg"". Must advance p.done.
void execInstruction(Process& p, int core) {
    if (p.done >= p.total) return;

    const Instruction& inst = p.instructions[p.done];

    switch (inst.op) {
        case OpCode::PRINT: {
            std::ostringstream ss;
            ss << "(" << now() << ") Core:" << core << " \"";
            
            if (!inst.var1.empty() && p.variables.count(inst.var1)) {
                ss << inst.msg << p.variables[inst.var1];
            } else {
                ss << inst.msg << p.name << "!";
            }
            
            ss << "\"";
            p.logs.push_back(ss.str());
            break;
        }
        case OpCode::DECLARE: {
            p.variables[inst.var1] = inst.val1;
            break;
        }
    }

    p.done++;
}