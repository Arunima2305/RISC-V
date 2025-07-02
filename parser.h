#ifndef PARSER_H
#define PARSER_H
#include <cstdint>
#include <string>
#include <vector>
#include "symbol_table.h"
using namespace std;
//define the struct Instruction
struct Instruction {
    string line_name;
    uint32_t address;       // Address of the instruction
    std::string assembly;  // Assembly instruction as a string
    std::string opcode="NULL";    // Opcode of the instruction
    std::string func3="NULL";     // Func3 field
    std::string func7="NULL";     // Func7 field
    std::string rd="NULL";        // Destination register
    std::string rs1="NULL";       // Source register 1
    std::string rs2="NULL";       // Source register 2
    std::string immediate="NULL"; // Immediate value
    std::string format;    // Instruction format (e.g., "R", "I", "S")
};
//parseFile function will take the filename, instructions and symbolTable as input and return a boolean value
bool parseFile(const std::string& filename, std::vector<Instruction>& instructions, SymbolTable& symbolTable,bool firstPass);
string trimWhitespace(const string& str);
string removeComments(const string& str);
#endif