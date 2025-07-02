import sys
import os
import re
import subprocess
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QTextEdit, QTableWidget, QTableWidgetItem,
    QLabel, QFileDialog
)
from PyQt5.QtCore import Qt

class RISCVSimulator:
    def __init__(self):
        # Path to your compiled simulator.exe
        script_dir = os.path.dirname(os.path.abspath(__file__))
        self.simulator_exe = os.path.join(script_dir, "simulator.exe")
        if not os.path.isfile(self.simulator_exe):
            print(f"Error: simulator.exe not found at {self.simulator_exe}")
            self.simulator_exe = None

        # Which .mc file to feed it?
        self.mc_file = None

        # Internal state
        self.registers = {}
        self.initialize_registers()
        self.full_output = ""
        self.cycles = []
        self.current_cycle = 0

    def initialize_registers(self):
        """Reset registers to their power‑on defaults."""
        self.registers = {f"x{i}": 0 for i in range(32)}
        self.registers["x0"] = 0
        self.registers["x2"] = 0x7FFFFFF0  # stack pointer example
        self.registers["x1"] = 0x00000000  # return address default

    def parse_registers(self, text):
        """
        Look for lines like:
            [WRITEBACK] Writing 0x6 to x10
        and update our register map.
        """
        for val_str, reg in re.findall(r"Writing\s+(0x[0-9A-Fa-f]+)\s+to\s+(x\d+)", text):
            try:
                self.registers[reg] = int(val_str, 16)
            except ValueError:
                pass

    def run_all(self):
        """
        Invoke simulator.exe (with the loaded .mc file if any),
        capture all its stdout, and parse every writeback.
        """
        if not self.simulator_exe:
            return "Error: simulator.exe not found."
        cmd = [self.simulator_exe]
        if self.mc_file:
            cmd.append(self.mc_file)
        try:
            proc = subprocess.run(cmd, capture_output=True, text=True)
            out = proc.stdout.strip() or "No output from simulator."
            self.parse_registers(out)
            return out
        except Exception as e:
            return f"Error running simulator: {e}"

    def run_full(self):
        """
        Run the full simulation, then split into cycle‑by‑cycle chunks
        so we can step through them.
        """
        self.full_output = self.run_all()
        parts = re.split(r"\n--------------------\n", self.full_output)
        self.cycles = [
            "--------------------\n" + p.strip()
            for p in parts if "[CYCLE" in p
        ]
        self.current_cycle = 0
        return self.full_output

    def step(self):
        """
        Return the next cycle’s output and update registers from only that cycle.
        If we’ve exhausted the pre‑split cycles, re‑run full to reset.
        """
        if self.current_cycle >= len(self.cycles):
            _ = self.run_full()
        if not self.cycles:
            return "No cycles to step through."
        block = self.cycles[self.current_cycle]
        self.parse_registers(block)
        self.current_cycle += 1
        return block

    def get_registers(self):
        return self.registers

    def reset(self):
        """Reset registers *and* clear previously captured cycles."""
        self.initialize_registers()
        self.full_output = ""
        self.cycles = []
        self.current_cycle = 0


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.simulator = RISCVSimulator()
        self.init_ui()

    def init_ui(self):
        self.setWindowTitle("RISC‑V Simulator GUI")
        self.setGeometry(50, 50, 1000, 700)

        main_layout = QVBoxLayout()

        # Header
        hdr = QLabel("🔹 RISC‑V Simulator GUI\nLoad your .mc, then Run or Step through cycles.")
        hdr.setAlignment(Qt.AlignCenter)
        main_layout.addWidget(hdr)

        # Buttons row
        row = QHBoxLayout()
        load_btn = QPushButton("Load .mc File")
        load_btn.clicked.connect(self.load_mc_file)
        row.addWidget(load_btn)

        run_btn = QPushButton("Run ▶")
        run_btn.clicked.connect(self.run_simulation)
        row.addWidget(run_btn)

        step_btn = QPushButton("Step ▶")
        step_btn.clicked.connect(self.step_simulation)
        row.addWidget(step_btn)

        reset_btn = QPushButton("Reset ↺")
        reset_btn.clicked.connect(self.reset_registers)
        row.addWidget(reset_btn)

        main_layout.addLayout(row)

        # Instruction Table
        main_layout.addWidget(QLabel("Instructions (PC + Machine Code):"))
        self.inst_table = QTableWidget(0, 2)
        self.inst_table.setHorizontalHeaderLabels(["PC", "Machine Code"])
        self.inst_table.horizontalHeader().setStretchLastSection(True)
        main_layout.addWidget(self.inst_table, stretch=1)

        # Registers Table
        main_layout.addWidget(QLabel("Registers:"))
        self.reg_table = QTableWidget(32, 2)
        self.reg_table.setHorizontalHeaderLabels(["Register", "Value"])
        self.reg_table.horizontalHeader().setStretchLastSection(True)
        main_layout.addWidget(self.reg_table, stretch=1)

        # Console Output
        main_layout.addWidget(QLabel("Simulator Output:"))
        self.output_box = QTextEdit()
        self.output_box.setReadOnly(True)
        main_layout.addWidget(self.output_box, stretch=2)

        self.setLayout(main_layout)
        self.update_register_table()

    def load_mc_file(self):
        path, _ = QFileDialog.getOpenFileName(
            self, "Open .mc file", "", "Machine Code Files (*.mc)")
        if not path:
            return

        # Tell the simulator about it
        self.simulator.mc_file = path

        # Load PC + raw code into the instruction table
        rows = []
        try:
            with open(path, 'r') as f:
                for line in f:
                    parts = line.strip().split()
                    if len(parts) >= 2:
                        rows.append((parts[0], parts[1]))
        except Exception as e:
            self.output_box.append(f"❌ Failed to parse .mc: {e}")
            return

        self.inst_table.setRowCount(len(rows))
        for r, (pc, code) in enumerate(rows):
            self.inst_table.setItem(r, 0, QTableWidgetItem(pc))
            self.inst_table.setItem(r, 1, QTableWidgetItem(code))

        self.output_box.append(f"📥 Loaded {len(rows)} instructions from:\n  {path}")

    def run_simulation(self):
        out = self.simulator.run_full()
        self.output_box.append(out)
        self.update_register_table()

    def step_simulation(self):
        out = self.simulator.step()
        self.output_box.append(out)
        self.update_register_table()

    def reset_registers(self):
        self.simulator.reset()
        self.output_box.append("🔄 Registers reset to defaults.")
        self.update_register_table()

    def update_register_table(self):
        regs = self.simulator.get_registers()
        # Sort by register number x0, x1, … x31
        for i, reg in enumerate(sorted(regs.keys(), key=lambda x: int(x[1:]))):
            self.reg_table.setItem(i, 0, QTableWidgetItem(reg))
            self.reg_table.setItem(i, 1, QTableWidgetItem(hex(regs[reg])))


def main():
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec_())


if __name__ == "__main__":
    main()
