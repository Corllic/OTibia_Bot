import random
import win32api
import win32con
import win32gui
from PyQt5.QtCore import QThread, Qt
from PyQt5.QtWidgets import QListWidgetItem

from Addresses import coordinates_x, coordinates_y
import Addresses
from Functions.MemoryFunctions import read_target_info, read_my_wpt, read_targeting_status
from Functions.MouseFunctions import mouse_function


from PyQt5.QtCore import QThread, Qt, pyqtSignal

class SetSmartHotkeyThread(QThread):
    status_signal = pyqtSignal(str, str) # text, style
    hotkey_set_signal = pyqtSignal(dict) # smart_hotkey_data

    def __init__(self, hotkey_name, rune_option):
        super().__init__()
        self.running = True
        self.hotkey_name = hotkey_name
        self.rune_option = rune_option

    def run(self):
        self.status_signal.emit("Move mouse to target location...", "color: blue; font-weight: bold;")
        while self.running:
            x, y = win32gui.ScreenToClient(Addresses.game, win32api.GetCursorPos())
            self.status_signal.emit(f"Current: X={x}  Y={y}", "color: blue; font-weight: bold;")
            
            if win32api.GetAsyncKeyState(win32con.VK_LBUTTON) & 0x8000:
                self.status_signal.emit(f"Coordinates set at X={x}, Y={y}", "color: green; font-weight: bold;")
                
                smart_hotkey_data = {
                    "Hotkey": self.hotkey_name,
                    "Option": self.rune_option,
                    "X": x,
                    "Y": y
                }
                self.hotkey_set_signal.emit(smart_hotkey_data)
                self.running = False
                return
            QThread.msleep(50)

    def stop(self):
        self.running = False


class SmartHotkeysThread(QThread):
    def __init__(self, hotkeys_data):
        super().__init__()
        self.running = True
        self.hotkeys_data = hotkeys_data
        # Czasy ostatniego wykonania dla kazdego hotkeya
        self.last_times = {}

    def run(self):
        import time
        while self.running:
            now = time.time()
            for i, hotkey_data in enumerate(self.hotkeys_data):
                if not self.running: break
                try:
                    hotkey_str = hotkey_data.get('Hotkey', '')
                    interval = float(hotkey_data.get('Interval', 5.0))
                    x = hotkey_data.get('X', 0)
                    y = hotkey_data.get('Y', 0)

                    # Sprawdz czy minql interwal
                    last = self.last_times.get(i, 0)
                    if now - last < interval:
                        continue

                    self.last_times[i] = now

                    # Wcisnij klawisz do okna gry (bez przejmowania myszki)
                    vk_code = None
                    if hotkey_str.upper().startswith('F') and hotkey_str[1:].isdigit():
                        vk_code = 111 + int(hotkey_str[1:])
                    elif hotkey_str.isdigit():
                        vk_code = ord(hotkey_str)
                    elif len(hotkey_str) == 1 and hotkey_str.isalpha():
                        vk_code = ord(hotkey_str.upper())
                    elif hotkey_str == '*':
                        vk_code = 0x6A

                    if vk_code:
                        win32gui.PostMessage(Addresses.game, win32con.WM_KEYDOWN, vk_code, 0x00010001)
                        win32gui.PostMessage(Addresses.game, win32con.WM_KEYUP,   vk_code, 0xC0010001)

                    # Kliknij na koordynaty (PostMessage - bez ruszania myszka!)
                    if x and y:
                        lp = win32api.MAKELONG(x, y)
                        win32gui.PostMessage(Addresses.game, win32con.WM_MOUSEMOVE,   0, lp)
                        win32gui.PostMessage(Addresses.game, win32con.WM_LBUTTONDOWN, 1, lp)
                        win32gui.PostMessage(Addresses.game, win32con.WM_LBUTTONUP,   0, lp)

                except Exception:
                    pass
            QThread.msleep(50)

    def stop(self):
        self.running = False

