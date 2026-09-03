"""
CatchTab - zakładka łapania Pokemonów
Reguły: Typ (Shiny/Normal/Obrazek) | Klawisz | Cooldown | Obrazek | Próg | Aktywny
"""
import json
import os

from PyQt5.QtWidgets import (
    QWidget, QGridLayout, QTableWidget, QPushButton, QLabel,
    QCheckBox, QLineEdit, QComboBox, QHeaderView, QFileDialog,
    QTableWidgetItem, QHBoxLayout
)
from PyQt5.QtGui import QIcon, QPixmap
from PyQt5.QtCore import Qt, QTimer

from Catch.CatchThread import CatchThread
from Functions.GeneralFunctions import manage_profile


class CatchTab(QWidget):
    def __init__(self):
        super().__init__()

        self.catch_thread = None

        self.setWindowIcon(QIcon('Images/Icon.jpg'))
        self.setWindowTitle("Catch Pokemon")
        self.setFixedSize(620, 380)

        self.status_label = QLabel("", self)
        self.status_label.setAlignment(Qt.AlignCenter)
        self.status_label.setStyleSheet("color: red; font-weight: bold;")

        self.layout = QGridLayout(self)
        self.setLayout(self.layout)

        # Tabela: Typ | Klawisz | Cooldown(ms) | Obrazek | Próg | Aktywny
        self.table = QTableWidget(self)
        self.table.setColumnCount(6)
        self.table.setHorizontalHeaderLabels([
            "Typ", "Klawisz", "Cooldown (ms)", "Obrazek", "Próg", "Aktywny"
        ])
        header = self.table.horizontalHeader()
        header.setSectionResizeMode(0, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(1, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(2, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(3, QHeaderView.Stretch)
        header.setSectionResizeMode(4, QHeaderView.ResizeToContents)
        header.setSectionResizeMode(5, QHeaderView.ResizeToContents)

        # Przyciski
        self.add_btn    = QPushButton("Dodaj regułę", self)
        self.remove_btn = QPushButton("Usuń", self)
        self.add_btn.clicked.connect(self.add_rule)
        self.remove_btn.clicked.connect(self.remove_rule)

        info = QLabel("Shiny/Normal: wykrywa przez OCR battle listy.  Obrazek: wykrywa przez screen.", self)
        info.setStyleSheet("color: #aaa; font-size: 10px;")
        info.setWordWrap(True)

        # Sync timer
        self.sync_timer = QTimer(self)
        self.sync_timer.timeout.connect(self._sync_to_thread)
        self.sync_timer.start(2000)

        # Layout
        self.layout.addWidget(self.table,      0, 0, 1, 2)
        self.layout.addWidget(self.add_btn,    1, 0)
        self.layout.addWidget(self.remove_btn, 1, 1)
        self.layout.addWidget(info,            2, 0, 1, 2)
        self.layout.addWidget(self.status_label, 3, 0, 1, 2)

        # Domyślna reguła – Shiny na klawisz 1
        self._insert_row(rule_type="shiny", key="1", cooldown="2500",
                         image_path="", threshold="0.85", active=True)

    # ── Dodawanie / usuwanie ──────────────────────────────────────────────────

    def add_rule(self):
        self._insert_row()

    def _insert_row(self, rule_type="shiny", key="1", cooldown="2500",
                    image_path="", threshold="0.85", active=False):
        row = self.table.rowCount()
        self.table.insertRow(row)

        # Kol 0: Typ
        type_combo = QComboBox()
        type_combo.addItems(["shiny", "normal", "image"])
        type_combo.setCurrentText(rule_type)
        self.table.setCellWidget(row, 0, type_combo)

        # Kol 1: Klawisz
        key_edit = QLineEdit(key)
        key_edit.setPlaceholderText("1, F1, *...")
        key_edit.setMaximumWidth(60)
        self.table.setCellWidget(row, 1, key_edit)

        # Kol 2: Cooldown
        cd_edit = QLineEdit(cooldown)
        cd_edit.setMaximumWidth(80)
        self.table.setCellWidget(row, 2, cd_edit)

        # Kol 3: Obrazek + przycisk wyboru
        img_widget = QWidget()
        img_layout = QHBoxLayout(img_widget)
        img_layout.setContentsMargins(2, 0, 2, 0)
        img_edit = QLineEdit(image_path)
        img_edit.setPlaceholderText("(opcjonalny – tylko dla trybu image)")
        img_btn = QPushButton("📁")
        img_btn.setMaximumWidth(28)
        img_btn.clicked.connect(lambda _, e=img_edit: self._browse_image(e))
        img_layout.addWidget(img_edit)
        img_layout.addWidget(img_btn)
        self.table.setCellWidget(row, 3, img_widget)
        img_widget._img_edit = img_edit  # referencja

        # Kol 4: Próg
        thr_edit = QLineEdit(threshold)
        thr_edit.setMaximumWidth(55)
        self.table.setCellWidget(row, 4, thr_edit)

        # Kol 5: Aktywny
        chk_container = QWidget()
        chk_layout = QGridLayout(chk_container)
        chk_layout.setContentsMargins(0, 0, 0, 0)
        chk = QCheckBox()
        chk.setChecked(active)
        chk_layout.addWidget(chk, 0, 0, Qt.AlignCenter)
        self.table.setCellWidget(row, 5, chk_container)

    def _browse_image(self, edit):
        path, _ = QFileDialog.getOpenFileName(
            self, "Wybierz obrazek Pokemon", "Images/",
            "Obrazki (*.png *.jpg *.bmp *.jpeg)"
        )
        if path:
            edit.setText(path)

    def remove_rule(self):
        rows = set(idx.row() for idx in self.table.selectedIndexes())
        if not rows:
            r = self.table.currentRow()
            if r >= 0:
                rows.add(r)
        for r in sorted(rows, reverse=True):
            self.table.removeRow(r)

    # ── Odczyt danych ─────────────────────────────────────────────────────────

    def _get_rules(self):
        rules = []
        for row in range(self.table.rowCount()):
            type_combo = self.table.cellWidget(row, 0)
            key_edit   = self.table.cellWidget(row, 1)
            cd_edit    = self.table.cellWidget(row, 2)
            img_widget = self.table.cellWidget(row, 3)
            thr_edit   = self.table.cellWidget(row, 4)
            chk_cont   = self.table.cellWidget(row, 5)

            rule_type = type_combo.currentText() if type_combo else "shiny"
            key       = key_edit.text().strip() if key_edit else "1"
            try:    cooldown = int(cd_edit.text()) if cd_edit else 2500
            except: cooldown = 2500
            img_edit  = getattr(img_widget, '_img_edit', None)
            image_path = img_edit.text().strip() if img_edit else ""
            try:    threshold = float(thr_edit.text()) if thr_edit else 0.85
            except: threshold = 0.85
            chk = chk_cont.findChild(QCheckBox) if chk_cont else None
            active = chk.isChecked() if chk else False

            rules.append({
                "type": rule_type,
                "key": key,
                "cooldown": cooldown,
                "image_path": image_path,
                "threshold": threshold,
                "enabled": active,
            })
        return rules

    # ── Wątek ────────────────────────────────────────────────────────────────

    def _sync_to_thread(self):
        if self.catch_thread and self.catch_thread.isRunning():
            self.catch_thread.rules = self._get_rules()

    def start_catch_thread(self, state):
        from PyQt5.QtCore import Qt as _Qt
        if state == _Qt.Checked:
            if self.catch_thread and self.catch_thread.isRunning():
                self.catch_thread.stop()
                self.catch_thread.wait(2000)
            self.catch_thread = CatchThread(self._get_rules())
            self.catch_thread.start()
            self.status_label.setStyleSheet("color: green; font-weight: bold;")
            self.status_label.setText("Catch: DZIAŁA")
        else:
            if self.catch_thread:
                self.catch_thread.stop()
                self.catch_thread.wait(2000)
                self.catch_thread = None
            self.status_label.setStyleSheet("color: red; font-weight: bold;")
            self.status_label.setText("Catch: STOP")

    # ── Zapis / odczyt profilu ────────────────────────────────────────────────

    def save_settings(self, profile_name):
        os.makedirs("Save/Catch", exist_ok=True)
        data = {"rules": self._get_rules()}
        if manage_profile("save", "Save/Catch", profile_name, data):
            self.status_label.setStyleSheet("color: green; font-weight: bold;")
            self.status_label.setText(f"Profil '{profile_name}' zapisany!")

    def load_settings(self, profile_name):
        filename = f"Save/Catch/{profile_name}.json"
        try:
            with open(filename, "r") as f:
                data = json.load(f)
            self.table.setRowCount(0)
            for rule in data.get("rules", []):
                self._insert_row(
                    rule_type  = rule.get("type", "shiny"),
                    key        = rule.get("key", "1"),
                    cooldown   = str(rule.get("cooldown", 2500)),
                    image_path = rule.get("image_path", ""),
                    threshold  = str(rule.get("threshold", 0.85)),
                    active     = rule.get("enabled", False),
                )
            self.status_label.setStyleSheet("color: green; font-weight: bold;")
            self.status_label.setText(f"Profil '{profile_name}' załadowany!")
        except FileNotFoundError:
            self.status_label.setText(f"Profil '{profile_name}' nie znaleziony.")

    def closeEvent(self, event):
        if self.catch_thread:
            self.catch_thread.stop()
            self.catch_thread.wait(2000)
        event.accept()
