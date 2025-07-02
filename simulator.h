#ifndef SIMULATOR_H
#define SIMULATOR_H

#include <map>
#include <string>
#include "cpu.h"
#include "symbol_table.h"
// Loads the machine code (.mc file) into a map: address -> instruction string.
std::map<uint32_t, std::string> loadMCFile(const std::string& filename);

// Main simulation loop that processes instructions step-by-step.
void simulate(std::map<uint32_t, std::string>& instructions, CPU& cpu,  SymbolTable &symbolTable);


// Dumps the data memory into an output file before halting.
void dumpMemory(const CPU& cpu, const std::string& filename);
// Load data‐segment contents into cpu.memory map
void initializeMemoryFromDataSegments(CPU& cpu, SymbolTable& symbolTable);


#endif
