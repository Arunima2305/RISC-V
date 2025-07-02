#include "simulator.h"
#include "symbol_table.h"  // For SymbolTable, DataSegment, DataEntry
#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <cstdint>
#include <map>
// ─── Add these right below your includes ─────────────────────────────────────────
constexpr uint32_t STACK_SIZE = 1 << 20;     // 1 MiB stack
constexpr uint32_t STACK_BASE = 0x8000'0000; // choose a high address as "top of stack"
constexpr uint32_t STACK_END  = STACK_BASE - STACK_SIZE;
// ────────────────────────────────────────────────────────────────────────────────


// ===== Helper: Sign-extends a value that has "bits" bits. =====
int32_t signExtend(uint32_t value, int bits) {
    int shift = 32 - bits;
    return (int32_t)(value << shift) >> shift;
}

// ===== Immediate extraction functions for different instruction types. =====
int32_t getITypeImm(uint32_t IR) {
    uint32_t imm = (IR >> 20) & 0xFFF;
    return signExtend(imm, 12);
}

int32_t getSTypeImm(uint32_t IR) {
    uint32_t imm = ((IR >> 25) << 5) | ((IR >> 7) & 0x1F);
    return signExtend(imm, 12);
}

int32_t getBTypeImm(uint32_t IR) {
    uint32_t imm = ((IR >> 31) << 12) | (((IR >> 7) & 0x1) << 11) |
                   (((IR >> 25) & 0x3F) << 5) | (((IR >> 8) & 0xF) << 1);
    return signExtend(imm, 13);
}

int32_t getUTypeImm(uint32_t IR) {
    return (int32_t)(IR & 0xFFFFF000);
}

int32_t getJTypeImm(uint32_t IR) {
    uint32_t imm = ((IR >> 31) << 20) | (((IR >> 12) & 0xFF) << 12) |
                   (((IR >> 20) & 0x1) << 11) | (((IR >> 21) & 0x3FF) << 1);
    return signExtend(imm, 21);
}

// ===== Load machine code from file (address -> instruction string). =====
std::map<uint32_t, std::string> loadMCFile(const std::string& filename) {
    std::ifstream infile(filename);
    std::map<uint32_t, std::string> instructions;
    std::string line;
    while (std::getline(infile, line)) {
        std::stringstream ss(line);
        std::string pc_str, instr_str;
        ss >> pc_str >> instr_str;
        uint32_t pc = std::stoul(pc_str, nullptr, 16);
        instructions[pc] = instr_str;
    }
    return instructions;
}

// ===== Dump data memory to a file (each memory word printed in hex). =====
void dumpMemory(const CPU& cpu, const std::string& filename) {
    std::ofstream outfile(filename);
    for (const auto& mem : cpu.memory) {
        outfile << "0x" << std::hex << mem.first << " 0x" 
                << std::setfill('0') << std::setw(8) << mem.second << "\n";
    }
    std::cout << "[DUMP] Data memory dumped to " << filename << "\n";
}

/* 
 * ===== Initialize CPU memory from Data Segments in the SymbolTable. =====
 * 
 * We assume each DataEntry has a 'value' (stored in an int) and a 'size' in bytes.
 * We unpack the value's bytes (little-endian) and store them into the CPU's memory map
 * at the correct addresses (word-aligned in a 32-bit word map).
 */
void initializeMemoryFromDataSegments(CPU &cpu,  SymbolTable &symbolTable) {
    for (const DataSegment &seg : symbolTable.dataSegments) {
        uint32_t addr = seg.startAddress;
        for (const DataEntry &entry : seg.contents) {
            // For each byte in this entry...
            for (uint32_t i = 0; i < entry.size; i++) {
                uint8_t byte = (entry.value >> (8 * i)) & 0xFF;
                uint32_t byteAddr = addr + i;
                uint32_t wordAddr = byteAddr & ~0x3;    // Word-aligned address
                uint32_t offset   = (byteAddr & 0x3);   // Byte offset in that word

                // Load the existing word from memory if present, else 0.
                uint32_t word = 0;
                if (cpu.memory.find(wordAddr) != cpu.memory.end()) {
                    word = cpu.memory[wordAddr];
                }

                // Clear the target byte and set it to 'byte'.
                word = (word & ~(0xFF << (offset * 8))) | ((byte & 0xFF) << (offset * 8));
                cpu.memory[wordAddr] = word;
            }
            addr += entry.size;
        }
    }
}

/*
 * ===== The 5-step simulation loop with data initialization. =====
 * 
 * We pass in the symbol table so that we can call initializeMemoryFromDataSegments
 * before running the instruction loop, ensuring that data for lw, lb, etc. is present.
 */
void simulate(std::map<uint32_t, std::string>& instructions, CPU& cpu,  SymbolTable &symbolTable) {
    // --- STEP 0: Initialize memory from the data segments. ---
    initializeMemoryFromDataSegments(cpu, symbolTable);
    
    // Optionally, verify with:
    // dumpMemory(cpu, "init_data_dump.mc");
    cpu.regFile[2] = STACK_BASE;  

    // (Optional) Pre-zero the stack pages if you want:
    for (uint32_t addr = STACK_END; addr < STACK_BASE; addr += 4) {
        cpu.memory[addr] = 0;
    }

    while (true) {
        std::cout << "\n--------------------\n";
        std::cout << "[CYCLE " << cpu.clock << "]\n";
        
        // ===== STEP 1: FETCH =====
        if (instructions.find(cpu.PC) == instructions.end()) {
            std::cout << "[INFO] No instruction at PC = 0x" 
                      << std::hex << cpu.PC 
                      << ". Simulation complete." << std::endl;
            break;
        }
        cpu.IR = std::stoul(instructions[cpu.PC], nullptr, 16);
        std::cout << "[FETCH] PC = 0x" << std::hex << cpu.PC 
                  << ", IR = 0x" << std::setfill('0') << std::setw(8) << cpu.IR << "\n";
    
        // ===== STEP 2: DECODE =====
        uint32_t opcode = cpu.IR & 0x7F;
        uint32_t rd     = (cpu.IR >> 7) & 0x1F;
        uint32_t funct3 = (cpu.IR >> 12) & 0x7;
        uint32_t rs1    = (cpu.IR >> 15) & 0x1F;
        uint32_t rs2    = (cpu.IR >> 20) & 0x1F;
        uint32_t funct7 = (cpu.IR >> 25) & 0x7F;
        int32_t  imm    = 0;
    
        std::cout << "[DECODE] opcode = 0x" << std::hex << opcode 
                  << ", rd = x" << std::dec << rd 
                  << ", rs1 = x" << rs1;
        if (opcode == 0x23 || opcode == 0x63 || opcode == 0x33) {
            std::cout << ", rs2 = x" << rs2;
        }
        std::cout << "\n";
    
        // Identify the immediate if used.
        if (opcode == 0x13 || opcode == 0x03 || opcode == 0x67) {  // I-type
            imm = getITypeImm(cpu.IR);
        } else if (opcode == 0x23) {                              // S-type
            imm = getSTypeImm(cpu.IR);
        } else if (opcode == 0x63) {                              // B-type
            imm = getBTypeImm(cpu.IR);
        } else if (opcode == 0x37 || opcode == 0x17) {            // U-type
            imm = getUTypeImm(cpu.IR);
        } else if (opcode == 0x6F) {                              // J-type
            imm = getJTypeImm(cpu.IR);
        }
    
        // ===== STEP 3: EXECUTE =====
        int32_t aluResult = 0;
        bool branchTaken  = false;
        uint32_t newPC    = cpu.PC + 4; // Next instruction by default
        bool writeback    = true;       // By default, result goes to register rd
    
        switch (opcode) {
            // -- R-Type (0x33) --
            case 0x33:
                if (funct7 == 0x00) {
                    // ADD, SLL, SLT, SLTU, XOR, SRL, OR, AND
                    if (funct3 == 0x0) { // ADD
                        aluResult = cpu.regFile[rs1] + cpu.regFile[rs2];
                        std::cout << "[EXECUTE] add x" << rd << " = x" << rs1 << " + x" << rs2 << "\n";
                    } else if (funct3 == 0x1) { // SLL
                        aluResult = cpu.regFile[rs1] << (cpu.regFile[rs2] & 0x1F);
                        std::cout << "[EXECUTE] sll x" << rd << "\n";
                    } else if (funct3 == 0x2) { // SLT
                        aluResult = (cpu.regFile[rs1] < cpu.regFile[rs2]) ? 1 : 0;
                        std::cout << "[EXECUTE] slt x" << rd << "\n";
                    } else if (funct3 == 0x3) { // SLTU
                        aluResult = ((uint32_t)cpu.regFile[rs1] < (uint32_t)cpu.regFile[rs2]) ? 1 : 0;
                        std::cout << "[EXECUTE] sltu x" << rd << "\n";
                    } else if (funct3 == 0x4) { // XOR
                        aluResult = cpu.regFile[rs1] ^ cpu.regFile[rs2];
                        std::cout << "[EXECUTE] xor x" << rd << "\n";
                    } else if (funct3 == 0x5) {
                        if (funct7 == 0x00) { // SRL
                            aluResult = (uint32_t)cpu.regFile[rs1] >> (cpu.regFile[rs2] & 0x1F);
                            std::cout << "[EXECUTE] srl x" << rd << "\n";
                        } else if (funct7 == 0x20) { // SRA
                            aluResult = cpu.regFile[rs1] >> (cpu.regFile[rs2] & 0x1F);
                            std::cout << "[EXECUTE] sra x" << rd << "\n";
                        }
                    } else if (funct3 == 0x6) { // OR
                        aluResult = cpu.regFile[rs1] | cpu.regFile[rs2];
                        std::cout << "[EXECUTE] or x" << rd << "\n";
                    } else if (funct3 == 0x7) { // AND
                        aluResult = cpu.regFile[rs1] & cpu.regFile[rs2];
                        std::cout << "[EXECUTE] and x" << rd << "\n";
                    }
                } else if (funct7 == 0x20 && funct3 == 0x0) { // SUB
                    aluResult = cpu.regFile[rs1] - cpu.regFile[rs2];
                    std::cout << "[EXECUTE] sub x" << rd << "\n";
                } else if (funct7 == 0x01) {
                    // M-extension: MUL, DIV, REM
                    if (funct3 == 0x0) { // MUL
                        aluResult = cpu.regFile[rs1] * cpu.regFile[rs2];
                        std::cout << "[EXECUTE] mul x" << rd << "\n";
                    } else if (funct3 == 0x4) { // DIV
                        if (cpu.regFile[rs2] == 0) {
                            aluResult = -1; // division by zero => -1
                            std::cout << "[EXECUTE] div x" << rd << " (div by zero)\n";
                        } else {
                            aluResult = cpu.regFile[rs1] / cpu.regFile[rs2];
                            std::cout << "[EXECUTE] div x" << rd << "\n";
                        }
                    } else if (funct3 == 0x6) { // REM
                        if (cpu.regFile[rs2] == 0) {
                            aluResult = cpu.regFile[rs1]; // remainder by zero => dividend
                            std::cout << "[EXECUTE] rem x" << rd << " (div by zero)\n";
                        } else {
                            aluResult = cpu.regFile[rs1] % cpu.regFile[rs2];
                            std::cout << "[EXECUTE] rem x" << rd << "\n";
                        }
                    }
                }
                break;
            
            // -- I-Type Arithmetic (0x13) --
            case 0x13:
                if (funct3 == 0x0) { // ADDI
                    aluResult = cpu.regFile[rs1] + imm;
                    std::cout << "[EXECUTE] addi x" << rd << "\n";
                } else if (funct3 == 0x2) { // SLTI
                    aluResult = (cpu.regFile[rs1] < imm) ? 1 : 0;
                    std::cout << "[EXECUTE] slti x" << rd << "\n";
                } else if (funct3 == 0x3) { // SLTIU
                    aluResult = ((uint32_t)cpu.regFile[rs1] < (uint32_t)imm) ? 1 : 0;
                    std::cout << "[EXECUTE] sltiu x" << rd << "\n";
                } else if (funct3 == 0x4) { // XORI
                    aluResult = cpu.regFile[rs1] ^ imm;
                    std::cout << "[EXECUTE] xori x" << rd << "\n";
                } else if (funct3 == 0x6) { // ORI
                    aluResult = cpu.regFile[rs1] | imm;
                    std::cout << "[EXECUTE] ori x" << rd << "\n";
                } else if (funct3 == 0x7) { // ANDI
                    aluResult = cpu.regFile[rs1] & imm;
                    std::cout << "[EXECUTE] andi x" << rd << "\n";
                } else if (funct3 == 0x1) { // SLLI
                    uint32_t shamt = cpu.IR >> 20; // lower 5 bits
                    aluResult = cpu.regFile[rs1] << (shamt & 0x1F);
                    std::cout << "[EXECUTE] slli x" << rd << "\n";
                } else if (funct3 == 0x5) { // SRLI/SRAI
                    uint32_t shamt = cpu.IR >> 20;
                    if (funct7 == 0x00) { // SRLI
                        aluResult = (uint32_t)cpu.regFile[rs1] >> (shamt & 0x1F);
                        std::cout << "[EXECUTE] srli x" << rd << "\n";
                    } else if (funct7 == 0x20) { // SRAI
                        aluResult = cpu.regFile[rs1] >> (shamt & 0x1F);
                        std::cout << "[EXECUTE] srai x" << rd << "\n";
                    }
                }
                break;
            
            // -- I-Type Load (0x03) --
            case 0x03: {
                uint32_t addr = cpu.regFile[rs1] + imm;
                std::cout << "[EXECUTE] Load from address 0x" << std::hex << addr << "\n";
                aluResult = addr;
                break;
            }
            
            // -- S-Type Store (0x23) --
            case 0x23: {
                uint32_t addr = cpu.regFile[rs1] + imm;
                aluResult = addr;
                std::cout << "[EXECUTE] Store to address 0x" << std::hex << addr << "\n";
                writeback = false;
                break;
            }
            
            // -- B-Type Branch (0x63) --
            case 0x63:
                if (funct3 == 0x0) { // BEQ
                    branchTaken = (cpu.regFile[rs1] == cpu.regFile[rs2]);
                    std::cout << "[EXECUTE] beq: Branch " << (branchTaken ? "taken" : "not taken") << "\n";
                } else if (funct3 == 0x1) { // BNE
                    branchTaken = (cpu.regFile[rs1] != cpu.regFile[rs2]);
                    std::cout << "[EXECUTE] bne: Branch " << (branchTaken ? "taken" : "not taken") << "\n";
                } else if (funct3 == 0x4) { // BLT
                    branchTaken = (cpu.regFile[rs1] < cpu.regFile[rs2]);
                    std::cout << "[EXECUTE] blt: Branch " << (branchTaken ? "taken" : "not taken") << "\n";
                } else if (funct3 == 0x5) { // BGE
                    branchTaken = (cpu.regFile[rs1] >= cpu.regFile[rs2]);
                    std::cout << "[EXECUTE] bge: Branch " << (branchTaken ? "taken" : "not taken") << "\n";
                } else if (funct3 == 0x6) { // BLTU
                    branchTaken = ((uint32_t)cpu.regFile[rs1] < (uint32_t)cpu.regFile[rs2]);
                    std::cout << "[EXECUTE] bltu: Branch " << (branchTaken ? "taken" : "not taken") << "\n";
                } else if (funct3 == 0x7) { // BGEU
                    branchTaken = ((uint32_t)cpu.regFile[rs1] >= (uint32_t)cpu.regFile[rs2]);
                    std::cout << "[EXECUTE] bgeu: Branch " << (branchTaken ? "taken" : "not taken") << "\n";
                }
                if (branchTaken) {
                    newPC = cpu.PC + imm;
                }
                writeback = false;
                break;
            
            // -- U-Type Instructions (LUI, AUIPC) --
            case 0x37: // LUI
                aluResult = imm;
                std::cout << "[EXECUTE] lui x" << rd << "\n";
                break;
            case 0x17: // AUIPC
                aluResult = cpu.PC + imm;
                std::cout << "[EXECUTE] auipc x" << rd << "\n";
                break;
            
            // -- J-Type (JAL) --
            case 0x6F: {
                aluResult = cpu.PC + 4;  // Return address
                newPC = cpu.PC + imm;
                std::cout << "[EXECUTE] jal: Jumping to 0x" << std::hex << newPC << "\n";
                break;
            }
            
            // -- I-Type JALR (0x67) --
            case 0x67: {
                aluResult = cpu.PC + 4;  // Return address
                newPC = (cpu.regFile[rs1] + imm) & ~1; // Clear LSB
                std::cout << "[EXECUTE] jalr: Jumping to 0x" << std::hex << newPC << "\n";
                break;
            }
            
            // -- Custom HALT (0x7F) --
            case 0x7F:
                std::cout << "[HALT] HALT instruction encountered. Stopping simulation.\n";
                dumpMemory(cpu, "data_memory_dump.mc");
                return;
            
            default:
                std::cerr << "[ERROR] Unsupported opcode: 0x" << std::hex << opcode << "\n";
                return;
        }
    
        // ===== STEP 4: MEMORY ACCESS =====
        if (opcode == 0x03) { // Load instructions
            uint32_t addr = aluResult;
            if (funct3 == 0x0) { // LB
                uint32_t word = cpu.memory[addr & ~0x3];
                uint8_t byte = (word >> ((addr & 0x3) * 8)) & 0xFF;
                aluResult = signExtend(byte, 8);
                std::cout << "[MEMORY] lb: Loaded byte 0x" << std::hex << (int)byte << "\n";
            } else if (funct3 == 0x1) { // LH
                uint32_t word = cpu.memory[addr & ~0x3];
                uint16_t half = (word >> ((addr & 0x2) * 8)) & 0xFFFF;
                aluResult = signExtend(half, 16);
                std::cout << "[MEMORY] lh: Loaded half 0x" << std::hex << half << "\n";
            } else if (funct3 == 0x2) { // LW
                aluResult = cpu.memory[addr];
                std::cout << "[MEMORY] lw: Loaded word 0x" << std::hex << aluResult << "\n";
            } else if (funct3 == 0x4) { // LBU
                uint32_t word = cpu.memory[addr & ~0x3];
                uint8_t byte = (word >> ((addr & 0x3) * 8)) & 0xFF;
                aluResult = byte; // zero-extended
                std::cout << "[MEMORY] lbu: Loaded byte 0x" << std::hex << (int)byte << "\n";
            } else if (funct3 == 0x5) { // LHU
                uint32_t word = cpu.memory[addr & ~0x3];
                uint16_t half = (word >> ((addr & 0x2) * 8)) & 0xFFFF;
                aluResult = half; // zero-extended
                std::cout << "[MEMORY] lhu: Loaded half 0x" << std::hex << half << "\n";
            }
        }
        if (opcode == 0x23) { // Store instructions
            uint32_t addr = aluResult;
            uint32_t data = cpu.regFile[rs2];
            if (funct3 == 0x0) { // SB
                uint32_t word = cpu.memory[addr & ~0x3];
                int shift = (addr & 0x3) * 8;
                word = (word & ~(0xFF << shift)) | ((data & 0xFF) << shift);
                cpu.memory[addr & ~0x3] = word;
                std::cout << "[MEMORY] sb: Stored byte 0x" << std::hex << (data & 0xFF)
                          << " at 0x" << addr << "\n";
            } else if (funct3 == 0x1) { // SH
                uint32_t word = cpu.memory[addr & ~0x3];
                int shift = (addr & 0x2) * 8;
                word = (word & ~(0xFFFF << shift)) | ((data & 0xFFFF) << shift);
                cpu.memory[addr & ~0x3] = word;
                std::cout << "[MEMORY] sh: Stored half 0x" << std::hex << (data & 0xFFFF)
                          << " at 0x" << addr << "\n";
            } else if (funct3 == 0x2) { // SW
                cpu.memory[addr] = data;
                std::cout << "[MEMORY] sw: Stored word 0x" << std::hex << data
                          << " at 0x" << addr << "\n";
            }
        }
    
        // ===== STEP 5: WRITEBACK =====
        if (writeback && rd != 0) {
            std::cout << "[WRITEBACK] Writing 0x" << std::hex << aluResult
                      << " to x" << std::dec << rd << "\n";
            cpu.regFile[rd] = aluResult;
        }
    
        cpu.PC = newPC;
        cpu.clock++;
    
        std::cout << "[STATE] PC = 0x" << std::hex << cpu.PC
                  << ", Clock = " << std::dec << cpu.clock << "\n";
    }
}
