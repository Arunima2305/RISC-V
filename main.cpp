#include <cstdint>
#include <iostream>
#include <fstream>
#include <vector>
#include <iomanip>  // for hex formatting
#include "parser.h"
#include "converter.h"
#include "symbol_table.h"

using namespace std;

// Helper function to produce a debug string for an instruction
string func(const Instruction& instr) {
    // Construct a quick debug representation
    return instr.opcode + "-" +
           instr.func3 + "-" +
           instr.func7 + "-" +
           instr.rd + "-" +
           instr.rs1 + "-" +
           instr.rs2 + "-" +
           instr.immediate;
}

int main() {
    SymbolTable symbolTable;
    vector<Instruction> instructions;

    string inputFilename = "input.asm";

    // --- Pass 1: Collect labels and directives ---
    if (!parseFile(inputFilename, instructions, symbolTable, true)) {
        cerr << "Error: Failed in Pass 1 (Label Collection)." << endl;
        return 1;
    }

    // --- Pass 2: Parse instructions fully ---
    if (!parseFile(inputFilename, instructions, symbolTable, false)) {
        cerr << "Error: Failed in Pass 2 (Instruction Parsing)." << endl;
        return 1;
    }

    ofstream outFile("output.mc");
    if (!outFile.is_open()) {
        cerr << "Error: Could not open output file." << endl;
        return 1;
    }

    // -----------------------------------------------------------------
    // PRINT DATA SEGMENTS (Only once!)
    // -----------------------------------------------------------------
    for (const auto &seg : symbolTable.dataSegments) {
        uint32_t dataAddr = seg.startAddress;
        for (const DataEntry &entry : seg.contents) {
            outFile << "0x" << std::hex << dataAddr << " "
                    << "0x" << std::setfill('0') << std::setw(8) << entry.value
                    << " # Data" << std::endl;
            dataAddr += entry.size; // Increment by the actual size (1, 2, 4, or 8 bytes).
        }
    }
    

    // -----------------------------------------------------------------
    // PRINT INSTRUCTIONS
    // -----------------------------------------------------------------
    // For illustration, let’s start instructions at address 0x0
    uint32_t instrAddress = 0x00000000;
    for (auto &instr : instructions) {
        uint32_t machineCode = convertToMachineCode(instr, symbolTable);
        string debugInfo = func(instr);

        outFile << "0x" << hex << instrAddress << " "
                << "0x" << setfill('0') << setw(8) << machineCode
                << " , " << instr.line_name
                << " # " << debugInfo << endl;

        instrAddress += 4; // each instruction is 4 bytes
    }

    cout << "Successfully converted input.asm to output.mc (data printed once)!" << endl;
    outFile.close();
    return 0;
}
