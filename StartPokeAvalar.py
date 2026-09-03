#!/usr/bin/env python3
"""
EasyBot dla PokeAvalar
======================
Bot automatycznie wykrywa klienta PokeAvalar i startuje.

Konfiguracja:
- Adresy pamięci: Save/Settings/addresses.json
- Profil healingu: Save/HealingAttack/pokeavalar_default.json
- Profil targetowania: Save/Targeting/pokeavalar_default.json
- Waypointy: Save/Waypoints/

Jak znaleźć adresy pamięci:
1. Otwórz Cheat Engine
2. Dołącz do procesu "Poke Avalar (DX9).exe"
3. Znajdź adres HP, MP, X, Y, Z swojej postaci
4. Wpisz je do Save/Settings/addresses.json
"""

import sys
import os

# Ustawienie Tesseract OCR
if getattr(sys, 'frozen', False):
    tesseract_path = os.path.join(sys._MEIPASS, 'Tesseract-OCR', 'tesseract.exe')
else:
    # Szukaj Tesseract w kilku miejscach
    candidates = [
        r'C:\Program Files\Tesseract-OCR\tesseract.exe',
        r'C:\Program Files (x86)\Tesseract-OCR\tesseract.exe',
        r'C:\Users\{}\AppData\Local\Programs\Tesseract-OCR\tesseract.exe'.format(os.getenv('USERNAME', '')),
    ]
    tesseract_path = next((p for p in candidates if os.path.exists(p)), candidates[0])

import pytesseract
pytesseract.pytesseract.tesseract_cmd = tesseract_path

from PyQt5.QtWidgets import QApplication
import Addresses

from General.SelectTibiaTab import SelectTibiaTab


def main():
    # Tworzenie wymaganych katalogów
    for folder in [
        "Images",
        "Save",
        "Save/Targeting",
        "Save/Settings",
        "Save/Waypoints",
        "Save/HealingAttack",
        "Save/SmartHotkeys",
        "Save/Hotkeys",
        "Save/Looting",
        "Save/Spell",
        "Images/PokeAvalar",
    ]:
        os.makedirs(folder, exist_ok=True)

    # Tworzenie domyślnego addresses.json jeśli nie istnieje
    addr_path = "Save/Settings/addresses.json"
    if not os.path.exists(addr_path):
        import json
        default = {
            "game_config": {
                "square_size": "64",
                "architecture": "32",
                "collect_threshold": "0.90"
            },
            "my_x": {"address": "", "offset": "", "type": "Int"},
            "my_y": {"address": "", "offset": "", "type": "Int"},
            "my_z": {"address": "", "offset": "", "type": "Short"},
            "my_hp": {"address": "", "offset": "", "type": "Short"},
            "my_hp_max": {"address": "", "offset": "", "type": "Short"},
            "my_mp": {"address": "", "offset": "", "type": "Short"},
            "my_mp_max": {"address": "", "offset": "", "type": "Short"},
            "attack": {"address": "", "offset": "", "type": "Int"},
            "target_x": {"offset": "", "type": "Int"},
            "target_y": {"offset": "", "type": "Int"},
            "target_z": {"offset": "", "type": "Short"},
            "target_hp": {"offset": "", "type": "Byte"},
            "target_name": {"offset": "", "type": "String"}
        }
        with open(addr_path, "w") as f:
            json.dump(default, f, indent=4)
        print(f"Stworzono domyślny {addr_path}")
        print("WAŻNE: Uzupełnij adresy pamięci w Save/Settings/addresses.json używając Cheat Engine!")

    app = QApplication([])
    app.setStyle('Fusion')
    app.setStyleSheet(Addresses.dark_theme)

    login_window = SelectTibiaTab()
    login_window.show()

    app.exec()


if __name__ == '__main__':
    main()
