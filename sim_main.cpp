#include "simulator.h"
#include "symbol_table.h"
#include "parser.h"  // Make sure you include your parser header as well.
#include <iostream> 
#include <iomanip>  // for hex formatting
#include "cpu.h"
int main() {
    // Create a symbol table and vector for instructions.
    SymbolTable symbolTable;
    std::vector<Instruction> instructions;
    
    // Parse the assembly source file (both passes).
    std::string inputFilename = "input.asm";
    if (!parseFile(inputFilename, instructions, symbolTable, true)) {
        std::cerr << "Error in Pass 1 (Label Collection)." << std::endl;
        return 1;
    }
    
    // Continue with CPU initialization...
    CPU cpu;
    cpu.PC = 0x0;
    cpu.clock = 0;
    for (int i = 0; i < 32; i++) {
        cpu.regFile[i] = 0;
    }
    
    // Load machine code, etc.
    auto instructionsMap = loadMCFile("output.mc");
    
    // Now symbolTable.dataSegments is populated, so memory initialization will work.
    std::cout << "Starting RISC-V simulation...\n";
    simulate(instructionsMap, cpu, symbolTable);
    
    // Optionally dump final memory state.
    dumpMemory(cpu, "final_memory_dump.mc");
    
    std::cout << "Simulation complete. Total clock cycles: " << cpu.clock << "\n";
    return 0;
}
