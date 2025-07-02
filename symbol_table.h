#ifndef SYMBOL_TABLE_H
#define SYMBOL_TABLE_H

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

// DataEntry: holds a stored value and its size in bytes.
struct DataEntry {
    int value;       // The stored data value (for simplicity, using int)
    uint32_t size;   // Number of bytes used (e.g., 1 for .byte, 2 for .half, 4 for .word, 8 for .dword)
};

// DataSegment: represents a block of data with a starting address.
struct DataSegment {
    uint32_t startAddress;
    std::vector<DataEntry> contents;
};

class SymbolTable {
public:
    // Existing declarations…
    void addLabel(const std::string& label, uint32_t address);
    uint32_t getAddress(const std::string& label) const;
    void addData(uint32_t address, int val);
    int getData(uint32_t address) const;
    void addGlobal(const std::string& symbol);
    bool isGlobal(const std::string& symbol) const;
    void addConstant(const std::string& name, int value);
    int getConstant(const std::string& name) const;

    // New: a vector of DataSegments.
    std::vector<DataSegment> dataSegments;
    
    // New: helper function to add a data entry to the current segment.
    void addDataToCurrentSegment(uint32_t& dataAddress, int value, uint32_t size);
    
private:
    std::map<std::string, uint32_t> table;
    std::map<uint32_t, int> dataSection;
    std::set<std::string> globalSymbols;
    std::map<std::string, int> constants;
};

#endif // SYMBOL_TABLE_H
