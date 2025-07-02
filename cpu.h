#ifndef CPU_H
#define CPU_H

#include <array>
#include <map>
#include <cstdint>

// CPU structure containing program counter, instruction register, clock counter,
// register file, temporary registers, and a memory map for data memory.
struct CPU {
    uint32_t PC = 0;      // Program Counter
    uint32_t IR = 0;      // Instruction Register
    uint32_t clock = 0;   // Clock cycles

    std::array<int32_t, 32> regFile = {0}; // x0 to x31; x0 is hardwired to 0.
    uint32_t RM = 0, RY = 0, RZ = 0;         // Temporary registers

    std::map<uint32_t, uint32_t> memory;     // Data memory (address -> 32-bit word)
};

#endif
