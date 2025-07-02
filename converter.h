#ifndef CONVERTER_H
#define CONVERTER_H
#include <cstdint>
#include <string>
#include <vector>
#include "symbol_table.h"
#include "parser.h"
using namespace std;

// Declare instructionMap without defining it
extern map<string, pair<string, string>> instructionMap;
uint32_t convertToMachineCode( Instruction& instruction, const SymbolTable& symbolTable);
string registerToBinary(const string& reg);
string immediateToBinary(int imm, int bits);

#endif