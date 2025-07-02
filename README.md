
# 🔧 RISC-V Assembler & Pipeline Simulator

A comprehensive three-phase project built as part of the **CS204: Computer Architecture** course at IIT Ropar. The project models the full execution lifecycle of a 32-bit RISC-V processor—from assembly translation to functional simulation and pipelined execution with hazard handling and GUI visualization.

---

## 📁 Project Structure

| Phase | Description |
|-------|-------------|
| **Phase 1** | Assembler: Converts RISC-V assembly code to machine code (`.asm` → `.mc`) |
| **Phase 2** | Functional Simulator: Executes machine code using the 5-stage instruction execution model |
| **Phase 3** | Pipelined Simulator: Simulates pipelined execution with forwarding, hazard detection, and GUI |

---

## 📜 Phase 1: RISC-V Assembler

### ✅ Features
- Parses `.asm` files and generates `.mc` machine code
- Supports **31 RISC-V 32-bit instructions** across R, I, S, SB, U, and UJ formats
- Handles labels and assembler directives:
  - `.text`, `.data`, `.word`, `.byte`, `.half`, `.asciz`
- Code and Data segments formatted like Venus:
```

0x0 0x003100B3 , add x1,x2,x3 # 0110011-000-0000000-00001-00010-00011
0x4 0x00A37293 , andi x5,x6,10 # 0010011-111-...

````

### 📂 Input/Output
- **Input:** `input.asm`
- **Output:** `output.mc` (code + data segment)

---

## ⚙️ Phase 2: Functional Simulator

### 🧠 Instruction Execution Model
Each instruction from `.mc` is processed through:
1. **Fetch**
2. **Decode**
3. **Execute**
4. **Memory Access**
5. **Writeback**

### 🧩 Components Modeled
- Program Counter (PC)
- Instruction Register (IR)
- Register File (x0–x31)
- Temporary Registers (e.g., RM, RY, etc.)
- Data Memory

### ⏱️ Clock Management
- Clock cycle increments after each instruction
- Message log printed at each stage
- Final cycle count reported

### 📤 Output
- Internal state printed after every stage
- Modified data memory written to `.mc` upon termination

### 🧪 Tested Programs
- Fibonacci
- Factorial
- Bubble Sort

---

## 🚀 Phase 3: Pipelined Execution & GUI

### 📌 Key Enhancements
- Implements classic **5-stage pipeline** with:
- **Inter-stage pipeline registers**
- **Data hazard detection** with:
  - Stalling
  - Data forwarding
- **Control hazard handling** with:
  - Dynamic 1-bit **branch prediction**
  - **Flushing** on mispredictions

- **Separate text and data memory**
- Extensive **runtime debug knobs**

### 🎚 Knobs (Debug Controls)
| Knob | Description |
|------|-------------|
| `Knob1` | Enable/disable pipelining |
| `Knob2` | Enable/disable data forwarding |
| `Knob3` | Print full register file per cycle |
| `Knob4` | Print pipeline register states each cycle |
| `Knob5` | Print pipeline state for a specific instruction |
| `Knob6` | Log Branch Prediction Unit (PC, PHT, BTB) |

### 📈 Output Stats
- Total Cycles
- Total Instructions Executed
- CPI (Cycles Per Instruction)
- ALU, Load/Store, and Control Instruction Counts
- Stalls, Hazards, and Mispredictions breakdown


---

## 🏗️ Build & Run

### Clone the Project

```bash
git clone https://github.com/Arunima2305/RISC-V.git
cd RISC-V
````

### Phase 1 (Assembler)

```bash

g++ assembler.cpp -o assembler
./assembler input.asm output.mc
```

### Phase 2 (Functional Simulator)

```bash

g++ simulator.cpp -o simulator
./simulator output.mc
```

### Phase 3 (Pipelined Simulator)

```bash

g++ pipeline.cpp -o pipeline
./pipeline output.mc --knob1 --knob2 --knob4
```









## 📃 License

This project is licensed under the MIT License. See [`LICENSE`](LICENSE) for more information.



