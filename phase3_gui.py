import sys
import json
import subprocess
from PyQt5.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QHBoxLayout,
    QPushButton, QLabel, QTableWidget, QTableWidgetItem,
    QGraphicsView, QGraphicsScene, QSizePolicy, QGroupBox, QMessageBox
)
from PyQt5.QtCore import Qt, pyqtSlot
from PyQt5.QtGui import QPen, QColor


class PipelineStageWidget(QGroupBox):
    def __init__(self, name):
        super().__init__(name)
        self.setFixedSize(120, 80)
        self.label = QLabel("---")
        self.label.setAlignment(Qt.AlignCenter)
        layout = QVBoxLayout()
        layout.addWidget(self.label)
        self.setLayout(layout)
        self.reset_style()

    def reset_style(self):
        self.setStyleSheet(
            "QGroupBox { background: lightgray; border: 2px solid gray; border-radius: 5px; }"
        )

    def set_instruction(self, text, color=None):
        self.label.setText(text)
        if color:
            self.setStyleSheet(
                f"QGroupBox {{ background: {color}; border: 2px solid gray; border-radius: 5px; }}"
            )
        else:
            self.reset_style()


class PredictorTable(QTableWidget):
    def __init__(self, headers):
        super().__init__(0, len(headers))
        self.setHorizontalHeaderLabels(headers)
        self.setSizePolicy(QSizePolicy.Expanding, QSizePolicy.Expanding)
        self.setMinimumHeight(200)

    def add_entry(self, row_data):
        row = self.rowCount()
        self.insertRow(row)
        for col, val in enumerate(row_data):
            self.setItem(row, col, QTableWidgetItem(str(val)))


class MainWindow(QWidget):
    def __init__(self):
        super().__init__()
        self.setWindowTitle("RISC-V Pipeline Simulator GUI")
        self.resize(1200, 800)
        self._build_ui()

    def _build_ui(self):
        main_layout = QVBoxLayout()

        # 1) Block diagram of 5 stages
        blocks_layout = QHBoxLayout()
        self.stages = {}
        for name in ('IF', 'ID', 'EX', 'MEM', 'WB'):
            w = PipelineStageWidget(name)
            blocks_layout.addWidget(w)
            self.stages[name] = w
        main_layout.addLayout(blocks_layout)

        # Graphics view for forwarding arrows
        self.scene = QGraphicsScene()
        self.view = QGraphicsView(self.scene)
        self.view.setFixedHeight(200)
        main_layout.addWidget(self.view)

        # 2+5) Cycle table + predictor tables
        mid_layout = QHBoxLayout()

        # Cycle-by-cycle table
        self.cycle_table = QTableWidget(0, 5)
        self.cycle_table.setHorizontalHeaderLabels(['IF', 'ID', 'EX', 'MEM', 'WB'])
        mid_layout.addWidget(self.cycle_table, 3)

        # Predictor Panel
        pred_layout = QVBoxLayout()
        pred_layout.addWidget(QLabel("1-bit Branch Predictor"))
        self.pht_table = PredictorTable(['PC', 'Bit'])
        self.btb_table = PredictorTable(['PC', 'Target', 'Pred'])
        pred_layout.addWidget(QLabel("Pattern History Table"))
        pred_layout.addWidget(self.pht_table)
        pred_layout.addWidget(QLabel("Branch Target Buffer"))
        pred_layout.addWidget(self.btb_table)
        mid_layout.addLayout(pred_layout, 1)

        main_layout.addLayout(mid_layout)

        # Run/Step buttons
        btn_layout = QHBoxLayout()
        self.run_btn = QPushButton('Run ▶')
        self.step_btn = QPushButton('Step ▶')
        btn_layout.addWidget(self.run_btn)
        btn_layout.addWidget(self.step_btn)
        main_layout.addLayout(btn_layout)

        self.setLayout(main_layout)

        # Connect signals
        self.step_btn.clicked.connect(self.on_step)
        self.run_btn.clicked.connect(self.on_run)

    def _clear_forwarding(self):
        self.scene.clear()

    def _draw_forwarding(self, forwarding):
        pen = QPen(QColor('green'), 3)
        for entry in forwarding:
            src = entry['from']
            dst = entry['to']
            w1 = self.stages.get(src)
            w2 = self.stages.get(dst)
            if not w1 or not w2:
                continue
            p1 = w1.mapTo(self.view, w1.rect().center())
            p2 = w2.mapTo(self.view, w2.rect().center())
            self.scene.addLine(p1.x(), p1.y(), p2.x(), p2.y(), pen)

    def _update_blocks_and_table(self, data):
        # extract hazards
        data_haz = [h['stage'] for h in data['hazards'] if h['type'].lower() == 'raw']
        ctrl_haz = [h['stage'] for h in data['hazards'] if h['type'].lower() == 'control']

        # update block diagram
        for stage, widget in self.stages.items():
            txt = data['pipeline'].get(stage, '---')
            color = None
            if stage in data_haz:
                color = 'orange'
            elif stage in ctrl_haz:
                color = 'red'
            widget.set_instruction(txt, color)

        # add to cycle table
        row = self.cycle_table.rowCount()
        self.cycle_table.insertRow(row)
        for col, stage in enumerate(['IF', 'ID', 'EX', 'MEM', 'WB']):
            instr = data['pipeline'].get(stage, '---')
            item = QTableWidgetItem(instr)
            if stage in data_haz:
                item.setBackground(QColor('orange'))
            if stage in ctrl_haz:
                item.setBackground(QColor('red'))
            self.cycle_table.setItem(row, col, item)

    def _update_predictor(self, data):
        # clear
        self.pht_table.setRowCount(0)
        self.btb_table.setRowCount(0)

        pht = data['predictor']['PHT']
        btb = data['predictor']['BTB']
        # PHT entries
        for pc_str, bit in pht.items():
            self.pht_table.add_entry([pc_str, bit])
        # BTB entries (add predicted bit from PHT if available)
        for pc_str, tgt_str in btb.items():
            pred = pht.get(pc_str, 0)
            self.btb_table.add_entry([pc_str, tgt_str, pred])

    def _run_simulator(self):
        exe = 'simulator3.exe' if sys.platform.startswith('win') else './simulator3'
        try:
            proc = subprocess.run(
                [exe, 'output.mc', '--json'],
                capture_output=True, text=True, check=True
            )
            lines = [l for l in proc.stdout.splitlines() if l.strip()]
            return [json.loads(l) for l in lines]
        except subprocess.CalledProcessError as e:
            QMessageBox.critical(self, "Simulation Error", e.stderr)
            return []

    @pyqtSlot()
    def on_step(self):
        all_cycles = self._run_simulator()
        if not all_cycles:
            return
        data = all_cycles[0]
        self._clear_forwarding()
        self._update_blocks_and_table(data)
        self._draw_forwarding(data['forwarding'])
        self._update_predictor(data)

    @pyqtSlot()
    def on_run(self):
        for data in self._run_simulator():
            self._clear_forwarding()
            self._update_blocks_and_table(data)
            self._draw_forwarding(data['forwarding'])
            self._update_predictor(data)


if __name__ == '__main__':
    app = QApplication(sys.argv)
    w = MainWindow()
    w.show()
    sys.exit(app.exec_())
