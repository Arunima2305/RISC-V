#include "converter.h"
#include "symbol_table.h"
#include <map>
#include <bitset>
#include <iostream>

using namespace std;
#include "parser.h"
#include "symbol_table.h"
#include "converter.h"

// Lookup table for func3 and func7 for all 31 required instructions
map<string, pair<string, string>> instructionMap = {
    // R-format instructions (func3, func7)
    {"add", {"000", "0000000"}}, {"sub", {"000", "0100000"}}, {"xor", {"100", "0000000"}},
    {"or", {"110", "0000000"}}, {"and", {"111", "0000000"}}, {"sll", {"001", "0000000"}},
    {"slt", {"010", "0000000"}}, {"sra", {"101", "0100000"}}, {"srl", {"101", "0000000"}},
    {"mul", {"000", "0000001"}}, {"div", {"100", "0000001"}}, {"rem", {"110", "0000001"}},

    // I-format instructions (func3, no func7 needed)
    {"addi", {"000", ""}}, {"andi", {"111", ""}}, {"ori", {"110", ""}},
    {"lb", {"000", ""}}, {"lh", {"001", ""}}, {"lw", {"010", ""}}, {"ld", {"011", ""}}, {"jalr", {"000", ""}},

    // S-format instructions (func3, no func7 needed)
    {"sb", {"000", ""}}, {"sh", {"001", ""}}, {"sw", {"010", ""}}, {"sd", {"011", ""}},

    // SB-format instructions (func3, no func7 needed)
    {"beq", {"000", ""}}, {"bne", {"001", ""}}, {"blt", {"100", ""}}, {"bge", {"101", ""}},

    // U-format instructions (No func3, no func7)
    {"lui", {"", ""}}, {"auipc", {"", ""}},

    // UJ-format instructions (No func3, no func7)
    {"jal", {"", ""}}};

// Helper function to convert register name to binary (e.g., "x1" -> "00001")
string registerToBinary(const string &reg) {
    int regNum = stoi(reg.substr(1)); 
    return bitset<5>(regNum).to_string(); // Convert to 5-bit binary
}
string immediateToBinary(int imm, int bits)
{
    // Always return a string with exactly 'bits' bits
    return bitset<32>(imm).to_string().substr(32 - bits, bits);
}


// Convert RISC-V assembly instruction to machine code
uint32_t convertToMachineCode( Instruction &instruction, const SymbolTable &symbolTable) {
    string opcode, func3, func7, rd, rs1, rs2, immediate;
    uint32_t machineCode = 0;

    // Lookup func3 and func7 from the map
    auto it = instructionMap.find(instruction.opcode);
    if (it != instructionMap.end()) {
        func3 = it->second.first;
        func7 = it->second.second;
    } else {
        cerr << "Error: Unknown instruction '" << instruction.assembly << "'" << endl;
        return 0;
    }

    // Extract other fields based on the instruction format
    if (instruction.format == "R") {
        // R-format: add, sub, xor, etc.
        rd = registerToBinary(instruction.rd);
        rs1 = registerToBinary(instruction.rs1);
        rs2 = registerToBinary(instruction.rs2);
        instruction.rd = rd;
        instruction.rs1=rs1;
        instruction.rs2=rs2;
        instruction.func3=func3;
        instruction.func7=func7;
        instruction.opcode="0110011";

        opcode = "0110011";
    } 
    else if (instruction.format == "I") {
        // I-format: addi, lw, jalr, etc.
        rd = registerToBinary(instruction.rd);
        rs1 = registerToBinary(instruction.rs1);
        // immediateToBinary now returns exactly 12 bits.
        immediate = immediateToBinary(stoi(instruction.immediate), 12);
        if (instruction.opcode == "jalr")
            opcode = "1100111";
        else if (instruction.opcode == "lw" || instruction.opcode == "lh" || instruction.opcode == "lb" || instruction.opcode == "ld")
            opcode = "0000011";
        else
            opcode = "0010011";
        instruction.rd = rd;
        instruction.rs1 = rs1;
        instruction.func3 = func3;
        // Remove the erroneous substring extraction:
        instruction.immediate = immediate; 
        instruction.opcode = opcode;
    }
    
    else if (instruction.format == "S") {
        // S-format: sw, sb, etc.
        rs1 = registerToBinary(instruction.rs1);
        rs2 = registerToBinary(instruction.rs2);
        immediate = immediateToBinary(stoi(instruction.immediate), 12);
        opcode = "0100011";        
        instruction.rs1=rs1;
        instruction.rs2=rs2;
        instruction.func3=func3;
        instruction.immediate=immediate;
        instruction.opcode=opcode;
    } 
    else if (instruction.format == "SB") {
        // SB-format: beq, bne, etc.
        rs1 = registerToBinary(instruction.rs1);
        rs2 = registerToBinary(instruction.rs2);
       // cout<<instruction.immediate<<endl;
        // Convert immediate (string) to int32_t offset
        immediate = std::bitset<13>(stoi(instruction.immediate)).to_string();
        cout<<immediate<<endl;
        opcode="1100011";
        instruction.rs1=rs1;
        instruction.rs2=rs2;
        instruction.func3=func3;
        instruction.immediate=immediate;
        instruction.opcode=opcode;
        
    }
    
    
    
    else if (instruction.format == "U") {
        // U-format: lui, auipc
        rd = registerToBinary(instruction.rd);
        immediate = immediateToBinary(stoi(instruction.immediate), 20);
        opcode = instruction.opcode == "lui" ? "0110111" : "0010111"; 
        instruction.rd=rd;        
        instruction.immediate=immediate;
        instruction.opcode=opcode;
    } 
    else if (instruction.format == "UJ") {
        rd = registerToBinary(instruction.rd);
        
        // Use the already computed offset (as a decimal string) from the parser.
        int offset = stoi(instruction.immediate);
        
        // The JAL immediate field encodes (offset / 2) into 20 bits.
        immediate = immediateToBinary(offset / 2, 20);
    
        opcode = "1101111";
        instruction.rd = rd;
        instruction.immediate = immediate;
        instruction.opcode = opcode;
    }
    

    // Combine fields into 32-bit machine code
    if (instruction.format == "R") {
        machineCode = stoul(func7 + rs2 + rs1 + func3 + rd + opcode, nullptr, 2);
    } 
    else if (instruction.format == "I") {
        machineCode = stoul(immediate + rs1 + func3 + rd + opcode, nullptr, 2);
    } 
    else if (instruction.format == "S") {
       // cout<<immediate<<endl;
        
        machineCode = stoul(immediate.substr(0, 7) + rs2 + rs1 + func3 + immediate.substr(7, 5) + opcode, nullptr, 2);
    } 
    else if (instruction.format == "SB") {
        machineCode = stoul(immediate[0] + immediate.substr(2, 6) + rs2 + rs1 + func3 +
                            immediate.substr(8, 4) + immediate[1] + opcode, nullptr, 2);
    } 
    else if (instruction.format == "U") {
        machineCode = stoul(immediate + rd + opcode, nullptr, 2);
    } 
    else if (instruction.format == "UJ") {
        
        machineCode = stoul(immediate[0] + immediate.substr(10, 10) + immediate[9] +
                            immediate.substr(1, 8) + rd + opcode, nullptr, 2);
    }

    return machineCode;
}
