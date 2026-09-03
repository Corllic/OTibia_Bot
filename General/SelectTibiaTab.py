import Addresses
from PyQt5.QtWidgets import (QWidget, QGridLayout, QPushButton, QListWidget, QLabel, QVBoxLayout)
from PyQt5.QtGui import QIcon
from General.MainWindowTab import MainWindowTab
import win32gui
import win32process
import psutil

# Słowa kluczowe tytułu okna klienta gry - zmień jeśli tytuł jest inny
AUTO_CONNECT_KEYWORDS = ["Avalar", "PokeAvalar", "Poke Avalar"]


class SelectTibiaTab(QWidget):
    def __init__(self):
        super().__init__()
        self.main_window = None
        self.process_list = []

        # Set window icon
        self.setWindowIcon(QIcon('Images/Icon.jpg'))
        self.setWindowTitle("EasyBot Select Client")
        self.setFixedSize(500, 400)

        # Layout
        self.layout = QVBoxLayout(self)

        # Status label
        self.status_label = QLabel("Szukam klienta PokeAvalar...", self)
        self.status_label.setStyleSheet("font-weight: bold; font-size: 14px;")
        self.layout.addWidget(self.status_label)

        # List widget (pokazywany tylko gdy auto-connect nie zadziała)
        self.process_listwidget = QListWidget(self)
        self.layout.addWidget(self.process_listwidget)

        # Connect button
        self.connect_button = QPushButton('Połącz z wybranym', self)
        self.connect_button.clicked.connect(self.load_tibia_button)
        self.layout.addWidget(self.connect_button)

        # Refresh button
        self.refresh_button = QPushButton('Odśwież listę', self)
        self.refresh_button.clicked.connect(self.refresh_processes)
        self.layout.addWidget(self.refresh_button)

        # Próbuj auto-connect przy starcie
        self.refresh_processes()

    def refresh_processes(self):
        """Enumerate all processes with windows and display them."""
        self.process_listwidget.clear()
        self.process_list = []

        def enum_window_callback(hwnd, _):
            if win32gui.IsWindowVisible(hwnd):
                window_text = win32gui.GetWindowText(hwnd)
                if window_text and "Easy Bot" not in window_text:
                    try:
                        _, proc_id = win32process.GetWindowThreadProcessId(hwnd)
                        process = psutil.Process(proc_id)
                        process_name = process.name()

                        self.process_list.append({
                            'hwnd': hwnd,
                            'window_title': window_text,
                            'proc_id': proc_id,
                            'process_name': process_name
                        })

                        display_text = f"{window_text} ({process_name} - PID: {proc_id})"
                        self.process_listwidget.addItem(display_text)
                    except (psutil.NoSuchProcess, psutil.AccessDenied):
                        pass

        win32gui.EnumWindows(enum_window_callback, None)

        # Szukaj pasujących klientów po słowach kluczowych
        matching = [
            p for p in self.process_list
            if any(kw.lower() in p['window_title'].lower() for kw in AUTO_CONNECT_KEYWORDS)
        ]

        if len(matching) == 1:
            # Dokładnie jeden klient PokeAvalar - połącz automatycznie
            self.status_label.setText(f"✓ Auto-połączono z: {matching[0]['window_title']}")
            self.status_label.setStyleSheet("font-weight: bold; font-size: 14px; color: #00ff88;")
            self._connect_to(matching[0])
        elif len(matching) > 1:
            self.status_label.setText(f"Znaleziono {len(matching)} klientów PokeAvalar - wybierz:")
            self.status_label.setStyleSheet("font-weight: bold; font-size: 14px; color: #ffaa00;")
            # Pokaż tylko pasujące
            self.process_listwidget.clear()
            self.process_list = matching
            for p in matching:
                self.process_listwidget.addItem(f"{p['window_title']} (PID: {p['proc_id']})")
        else:
            self.status_label.setText("Nie znaleziono PokeAvalar - wybierz klienta ręcznie:")
            self.status_label.setStyleSheet("font-weight: bold; font-size: 14px; color: #ff6666;")

    def _connect_to(self, proc):
        """Łączy z wybranym procesem i otwiera główne okno bota."""
        Addresses.load_tibia(
            window_title=proc['window_title'],
            proc_id=proc['proc_id'],
            hwnd=proc['hwnd']
        )
        self.close()
        self.main_window = MainWindowTab()
        self.main_window.show()

    def load_tibia_button(self) -> None:
        selected_index = self.process_listwidget.currentRow()
        if selected_index < 0 or selected_index >= len(self.process_list):
            return

        self._connect_to(self.process_list[selected_index])

