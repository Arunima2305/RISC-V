#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <algorithm>
#include <cstdint>
#include "parser.h"
#include "symbol_table.h"
#include "converter.h"

using namespace std;

// Helper function to trim whitespace from a string.
std::string trimWhitespace(const std::string& str) {
    size_t first = str.find_first_not_of(" \t");
    if (first == std::string::npos)
        return "";
    size_t last = str.find_last_not_of(" \t");
    return str.substr(first, last - first + 1);
}

// Helper function to remove comments (anything after '#').
string removeComments(const string& str) {
    size_t commentPos = str.find('#');
    if (commentPos != string::npos) {
        return str.substr(0, commentPos);
    }
    return str;
}

string decimalToHex(int decimal) {
    stringstream ss;
    ss << hex << decimal;  
    return ss.str();
}

// Function to parse instruction fields.
void parseInstructionFields(const std::string& line, std::string& line_name,
    std::string& opcode, std::string& format, std::string& rd, std::string& rs1,
    std::string& rs2, std::string& immediate)
{
    line_name = line;
    istringstream iss(line);
    iss >> opcode;  // Read the opcode first

    // R-format instructions
    if (opcode == "add" || opcode == "sub" || opcode == "xor" || 
        opcode == "or"  || opcode == "and" || opcode == "sll" || 
        opcode == "slt" || opcode == "sra" || opcode == "srl" ||
        opcode == "mul" || opcode == "div" || opcode == "rem")
    {
        format = "R";
        iss >> rd >> rs1 >> rs2;
        if (!rd.empty() && rd.back() == ',') rd.pop_back();
        if (!rs1.empty() && rs1.back() == ',') rs1.pop_back();
        if (!rs2.empty() && rs2.back() == ',') rs2.pop_back();
    }
    // I-format for addi, andi, ori, and jalr (written as "jalr rd rs1 immediate")
    else if (opcode == "addi" || opcode == "andi" || opcode == "ori" || opcode == "jalr")
    {
        format = "I";
        iss >> rd >> rs1 >> immediate;
        if (!rd.empty() && rd.back() == ',') rd.pop_back();
        if (!rs1.empty() && rs1.back() == ',') rs1.pop_back();
    }
    // I-format for load instructions with offset(rs1) syntax.
    else if (opcode == "lb" || opcode == "lh" || opcode == "lw" || opcode == "ld")
    {
        format = "I";
        string rdStr, addressStr;
        iss >> rdStr >> addressStr;
        if (!rdStr.empty() && rdStr.back() == ',') rdStr.pop_back();
        size_t openBracket = addressStr.find('(');
        size_t closeBracket = addressStr.find(')');
        if (openBracket != string::npos && closeBracket != string::npos)
        {
            immediate = addressStr.substr(0, openBracket);
            rs1 = addressStr.substr(openBracket + 1, closeBracket - openBracket - 1);
        }
        rd = rdStr;
    }
    // S-format instructions: sb, sh, sw, sd (with offset(rs1) syntax).
    else if (opcode == "sb" || opcode == "sh" || opcode == "sw" || opcode == "sd")
    {
        format = "S";
        string rs2Str, addressStr;
        iss >> rs2Str >> addressStr;
        if (!rs2Str.empty() && rs2Str.back() == ',') rs2Str.pop_back();
        size_t openBracket = addressStr.find('(');
        size_t closeBracket = addressStr.find(')');
        if (openBracket != string::npos && closeBracket != string::npos)
        {
            immediate = addressStr.substr(0, openBracket);
            rs1 = addressStr.substr(openBracket + 1, closeBracket - openBracket - 1);
        }
        rs2 = rs2Str;
    }
    // SB-format instructions: beq, bne, blt, bge (e.g., "beq rs1, rs2, label")
    else if (opcode == "beq" || opcode == "bne" || opcode == "blt" || opcode == "bge")
    {
        format = "SB";
        iss >> rs1 >> rs2 >> immediate;
        cout << "Immediate: " << immediate << endl;
        if (!rs1.empty() && rs1.back() == ',') rs1.pop_back();
        if (!rs2.empty() && rs2.back() == ',') rs2.pop_back();
    }
    // U-format instructions: lui, auipc.
    else if (opcode == "lui" || opcode == "auipc")
    {
        format = "U";
        iss >> rd >> immediate;
        if (immediate[1] != 'x')
        {
            // Convert decimal to hex.
            immediate = decimalToHex(stoi(immediate));
            immediate = "0x" + immediate;
        }
        immediate = to_string(stoi(immediate, nullptr, 16));
        if (!rd.empty() && rd.back() == ',') rd.pop_back();
    }
    // UJ-format instructions: jal.
    else if (opcode == "jal")
    {
        format = "UJ";
        iss >> rd >> immediate;
        if (!rd.empty() && rd.back() == ',') rd.pop_back();
    }
}

// Function to handle assembler directives.
// Directives are processed only during the first pass.
void processDirective(const std::string& directive, std::istringstream& iss,
    SymbolTable& symbolTable, uint32_t& dataAddress) {
if (directive == ".text") {
// Switching to text section: no data changes.
}
else if (directive == ".data") {
// Start a new data segment.
DataSegment newSeg;
newSeg.startAddress = dataAddress;
symbolTable.dataSegments.push_back(newSeg);
}
else if (directive == ".word") {
int value;
// Each word is 4 bytes.
while (iss >> value) {
symbolTable.addDataToCurrentSegment(dataAddress, value, 4);
}
}
else if (directive == ".half") {
int value;
// Each halfword is 2 bytes.
while (iss >> value) {
// Ensure a segment exists.
if (symbolTable.dataSegments.empty()) {
DataSegment seg;
seg.startAddress = dataAddress;
symbolTable.dataSegments.push_back(seg);
}
// Store the lower 16 bits.
symbolTable.addDataToCurrentSegment(dataAddress, value & 0xFFFF, 2);
}
}
else if (directive == ".byte") {
int value;
// Each byte is 1 byte.
while (iss >> value) {
if (symbolTable.dataSegments.empty()) {
DataSegment seg;
seg.startAddress = dataAddress;
symbolTable.dataSegments.push_back(seg);
}
symbolTable.addDataToCurrentSegment(dataAddress, value & 0xFF, 1);
}
}
else if (directive == ".dword") {
int64_t value;
// Each double word is 8 bytes.
while (iss >> value) {
if (symbolTable.dataSegments.empty()) {
DataSegment seg;
seg.startAddress = dataAddress;
symbolTable.dataSegments.push_back(seg);
}
// For simplicity, store it as an int while using size 8.
symbolTable.addDataToCurrentSegment(dataAddress, static_cast<int>(value), 8);
}
}
else if (directive == ".asciiz") {
std::string str;
// Read the remainder of the line (string literal).
getline(iss, str);
str = trimWhitespace(str);
if (!str.empty() && str.front() == '"' && str.back() == '"') {
// Remove the surrounding quotes.
str = str.substr(1, str.size() - 2);
}
// Store each character as a 1-byte value.
for (char ch : str) {
if (symbolTable.dataSegments.empty()) {
DataSegment seg;
seg.startAddress = dataAddress;
symbolTable.dataSegments.push_back(seg);
}
symbolTable.addDataToCurrentSegment(dataAddress, ch, 1);
}
// Add a null terminator.
if (symbolTable.dataSegments.empty()) {
DataSegment seg;
seg.startAddress = dataAddress;
symbolTable.dataSegments.push_back(seg);
}
symbolTable.addDataToCurrentSegment(dataAddress, 0, 1);
}
else if (directive == ".globl") {
std::string symbol;
iss >> symbol;
// Optionally: symbolTable.addGlobal(symbol);
}
}

// Modified parseFile function.
// Directives (lines starting with '.') are processed only on the first pass,
// while instructions are parsed only on the second pass.
// Also, the text (instruction) address counter is increased only for instruction lines.
bool parseFile(const std::string& filename, std::vector<Instruction>& instructions,
               SymbolTable& symbolTable, bool firstPass)
{
    ifstream inFile(filename);
    if (!inFile.is_open()) {
        cerr << "Error: Could not open file " << filename << endl;
        return false;
    }

    std::string line;
    uint32_t textAddress = 0;           // Instruction memory address.
    uint32_t dataAddress = 0x10000000;    // Data section starts here.

    while(getline(inFile, line)) {
        line = removeComments(line);
        line = trimWhitespace(line);
        if (line.empty()) continue;

        // Process label if a colon is present.
        size_t colonPos = line.find(':');
        if (colonPos != std::string::npos) {
            std::string label = trimWhitespace(line.substr(0, colonPos));
            if (!label.empty() && !isdigit(label[0])) {
                // Decide which address to store for the label.
                std::string rest = "";
                if (colonPos + 1 < line.size())
                    rest = trimWhitespace(line.substr(colonPos + 1));
                // If the rest of the line starts with '.', assume it's a data directive;
                // otherwise assume it is an instruction.
                if (!rest.empty() && rest[0] == '.')
                    if (firstPass)
                        symbolTable.addLabel(label, dataAddress);
                    else;
                else
                    if (firstPass)
                        symbolTable.addLabel(label, textAddress);
                    else;
            } else {
                cerr << "Error: Invalid label '" << label << "' - Labels cannot start with numbers!" << endl;
                return false;
            }
            // Remove the label part from the line.
            if (colonPos + 1 < line.size())
                line = trimWhitespace(line.substr(colonPos + 1));
            else
                continue;
        }

        if(line.empty())
            continue;

        // Process the current line.
        istringstream iss(line);
        string firstWord;
        iss >> firstWord;

        // If the line is a directive.
        if (!firstWord.empty() && firstWord[0] == '.') {
            // Process directives only during the first pass.
            if (firstPass) {
                processDirective(firstWord, iss, symbolTable, dataAddress);
            }
        }
        else {
            // Process instruction lines only during the second pass.
            if (!firstPass) {
                Instruction instr;
                parseInstructionFields(line, instr.line_name, instr.opcode, instr.format,
                                       instr.rd, instr.rs1, instr.rs2, instr.immediate);
                // For branch/jump instructions: resolve label immediates.
                if ((instr.format == "SB" || instr.format == "UJ") && !instr.immediate.empty())
                {
                    std::string lbl = trimWhitespace(instr.immediate);
                    if (!isdigit(lbl[0])) {
                        uint32_t labelAddress = symbolTable.getAddress(lbl);
                        if (labelAddress != 0xFFFFFFFF) {
                            int32_t offset = static_cast<int32_t>(labelAddress) - static_cast<int32_t>(textAddress);
                            instr.immediate = to_string(offset);
                            cout << "Resolved label '" << lbl << "' to offset " << instr.immediate << endl;
                        } else {
                            cerr << "Error: Label '" << lbl << "' not found in symbol table." << endl;
                            return false;
                        }
                    }
                }
                instructions.push_back(instr);
            }
            // Increase the text address only when an instruction is encountered.
            textAddress += 4;
        }
    }
    inFile.close();
    return true;
}
