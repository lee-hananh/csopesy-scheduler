#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdint>
#include "emulator.h"

// max for loop nesting depth
static const int MAX_FOR_DEPTH = 3;

// variable names pool to be used by the randomly generated instructions
static const char* VAR_POOL[] = {"var", "x", "y", "z"};

// returns a random variable name from the pool
static std::string randomVar() {
    return VAR_POOL[randomInt(0, 3)];
}

// randomly makes add/subtract operand
static void setOperand(std::string& var, uint16_t& val, bool& isVal) {
    if (randomInt(0, 1) == 0) {
        var = randomVar();
        isVal = false;
    } else {
        val = static_cast<uint16_t>(randomInt(0, 100));
        isVal = true;
    }
}

// creates one random instruction (except for loop)
static Instruction createRandomInstruction() {
    Instruction inst;
    int type = randomInt(0, 4);

    switch (type) {
        case 0:
            // PRINT
            inst.op = OpCode::PRINT;
            if (randomInt(0, 1) == 0) {
                inst.msg = "Hello world from ";
                inst.var1 = "";
            } else {
                inst.msg = "Value from var: ";
                inst.var1 = "var";
            }
            break;
        case 1:
            // DECLARE
            inst.op = OpCode::DECLARE;
            inst.var1 = randomVar();
            inst.val1 = static_cast<uint16_t>(randomInt(1, 100));
            break;
        case 2:
        case 3:
            // ADD/SUBTRACT
            inst.op = (type == 2) ? OpCode::ADD : OpCode::SUBTRACT;
            inst.var1 = randomVar();
            setOperand(inst.var2, inst.val2, inst.isVal2);
            setOperand(inst.var3, inst.val3, inst.isVal3);
            break;
        default:
            // SLEEP
            inst.op = OpCode::SLEEP;
            inst.val1 = static_cast<uint16_t>(randomInt(1, 5));
            break;
    }
    return inst;
}

// creates a random for instruction
static Instruction createForInstruction(int depth) {
    Instruction inst;
    inst.op = OpCode::FOR_LOOP;
    inst.repeats = randomInt(1, 3);
    int n = randomInt(1, 3);
    for (int i = 0; i < n; ++i) {
        if (depth < MAX_FOR_DEPTH && randomInt(0, 4) == 0)
            inst.body.push_back(createForInstruction(depth + 1));
        else
            inst.body.push_back(createRandomInstruction());
    }
    return inst;
}

// returns how many instructions was contributed once expanded
// all instructions aside from for count as 1, for counts as body size multiplied by repeats
static int expandedSize(const Instruction& inst) {
    if (inst.op != OpCode::FOR_LOOP) return 1;
    int s = 0;
    for (const auto& b : inst.body) s += expandedSize(b);
    return s * inst.repeats;
}

// expands for loop into flat sequence (body repeated by repeats value)
// all other instructions are appended without changes
static void flatten(const Instruction& inst, std::vector<Instruction>& out) {
    if (inst.op != OpCode::FOR_LOOP) {
        out.push_back(inst);
        return;
    }
    for (int r = 0; r < inst.repeats; ++r)
        for (const auto& b : inst.body) flatten(b, out);
}

// TODO: build the process's instruction list (PRINT, DECLARE, ADD,
// SUBTRACT, SLEEP, FOR). Called once when a process is created.
// Set p.total to the number of instructions.
void generateInstructions(Process& p) {
    p.instructions.clear();
    p.variables.clear();
    p.sleepTicks = 0;

    int count = randomInt(cfg.minIns, cfg.maxIns);
    while (static_cast<int>(p.instructions.size()) < count) {
        int remaining = count - static_cast<int>(p.instructions.size());
        Instruction inst;
        if (randomInt(0, 5) == 0) {
            inst = createForInstruction(1);
            // to avoid overshooting the target length.
            if (expandedSize(inst) > remaining) inst = createRandomInstruction();
        } else {
            inst = createRandomInstruction();
        }
        flatten(inst, p.instructions);
    }

    p.total = static_cast<int>(p.instructions.size());
    p.done = 0;
}

// reads an add/subtract operand
static uint16_t getOperand(Process& p, bool isVal, const std::string& var, uint16_t val) {
    if (isVal) return val;
    return p.variables[var];
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
        case OpCode::ADD: {
            uint32_t a = getOperand(p, inst.isVal2, inst.var2, inst.val2);
            uint32_t b = getOperand(p, inst.isVal3, inst.var3, inst.val3);
            p.variables[inst.var1] = static_cast<uint16_t>(std::min<uint32_t>(a + b, UINT16_MAX));
            break;
        }
        case OpCode::SUBTRACT: {
            uint16_t a = getOperand(p, inst.isVal2, inst.var2, inst.val2);
            uint16_t b = getOperand(p, inst.isVal3, inst.var3, inst.val3);
            p.variables[inst.var1] = (a > b) ? static_cast<uint16_t>(a - b) : 0;
            break;
        }
        case OpCode::SLEEP: {
            p.sleepTicks = static_cast<uint8_t>(inst.val1);
            break;
        }
        case OpCode::FOR_LOOP:
            break;
    }

    p.done++;
}