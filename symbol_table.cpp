#include "symbol_table.h"
#include <iostream>
#include <iomanip> // for hex if needed

using namespace std;

// Add a label and its address to the symbol table
void SymbolTable::addLabel(const string& label, uint32_t address) {
    table[label] = address;
}

// Retrieve the address of a label
uint32_t SymbolTable::getAddress(const string& label) const {
    auto it = table.find(label);
    if (it != table.end()) {
        return it->second;
    }
    return 0xFFFFFFFF; // Invalid address if not found.
}

// Add a data value at a specific address (for flat lookup)
void SymbolTable::addData(uint32_t address, int val) {
    dataSection[address] = val;
}

// Retrieve a data value from a specific address.
int SymbolTable::getData(uint32_t address) const {
    auto it = dataSection.find(address);
    if (it != dataSection.end()) {
        return it->second;
    }
    return -1; // Default if not found.
}

// Add a global symbol.
void SymbolTable::addGlobal(const string& symbol) {
    globalSymbols.insert(symbol);
}

// Check if a symbol is global.
bool SymbolTable::isGlobal(const string& symbol) const {
    return globalSymbols.find(symbol) != globalSymbols.end();
}

// Add a constant definition.
void SymbolTable::addConstant(const string& name, int value) {
    constants[name] = value;
}

// Retrieve a constant value.
int SymbolTable::getConstant(const string& name) const {
    auto it = constants.find(name);
    if (it != constants.end()) {
        return it->second;
    }
    return 0; // Default if not found.
}

// New: Add a data value to the current data segment.
// This function updates the provided dataAddress by the size of the value.
// Here we assume each value is stored as an int (typically 4 bytes).
void SymbolTable::addDataToCurrentSegment(uint32_t& dataAddress, int value, uint32_t size) {
    if (dataSegments.empty()) {
        DataSegment seg;
        seg.startAddress = dataAddress;
        dataSegments.push_back(seg);
    }
    DataEntry entry;
    entry.value = value;
    entry.size = size;
    dataSegments.back().contents.push_back(entry);
    dataAddress += size;
}
