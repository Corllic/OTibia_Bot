import random
import time
import win32api
import win32gui
import Addresses
import win32con
from PyQt5.QtCore import QThread, Qt, QMutex, QMutexLocker
from PyQt5.QtWidgets import QCheckBox, QLineEdit, QComboBox

class HotkeysThread(QThread):
    def __init__(self, hotkey_data_list=None):
        super().__init__()
        self.running = True
        self.hotkey_data_list = hotkey_data_list if hotkey_data_list else []
        self.last_execution_times = {}
        self.next_delays = {}
        self.data_lock = QMutex()

    def update_hotkey_data(self, new_data):
        with QMutexLocker(self.data_lock):
            self.hotkey_data_list = new_data
            # We might want to clear execution times if the list changed significantly,
            # but let's keep it simple for now.

    def run(self):
        while self.running:
            current_time = time.time()
            
            try:
                with QMutexLocker(self.data_lock):
                    current_list = list(self.hotkey_data_list)
                
                for index, entry in enumerate(current_list):
                    if not entry.get("Active", False):
                        continue
                    
                    hotkey_name = entry.get("Hotkey")
                    interval = entry.get("Interval", 2.0)
                    randomize = entry.get("Randomize", 0.5)
                    
                    if not hotkey_name:
                        continue
                    
                    # Initialize last execution time if not present
                    if index not in self.last_execution_times:
                        self.last_execution_times[index] = current_time
                        self.next_delays[index] = interval + random.uniform(0, randomize)
                        continue
                    
                    last_time = self.last_execution_times[index]
                    
                    # Check if enough time has passed
                    if current_time - last_time >= self.next_delays[index]:
                        # Execute Hotkey
                        self.press_hotkey(hotkey_name)

                        # Kliknij na koordynaty jesli ustawione (PostMessage - bez ruszania mysza)
                        click_x = entry.get("ClickX")
                        click_y = entry.get("ClickY")
                        if click_x is not None and click_y is not None:
                            import win32api as _w32a
                            lp = _w32a.MAKELONG(click_x, click_y)
                            win32gui.PostMessage(Addresses.game, win32con.WM_MOUSEMOVE,   0, lp)
                            win32gui.PostMessage(Addresses.game, win32con.WM_LBUTTONDOWN, 1, lp)
                            win32gui.PostMessage(Addresses.game, win32con.WM_LBUTTONUP,   0, lp)
                        
                        # Update time and calculate next delay
                        self.last_execution_times[index] = current_time
                        self.next_delays[index] = interval + random.uniform(0, randomize)
            except Exception as e:
                print(f"HotkeysThread error: {e}")
            
            QThread.msleep(10)


    def press_hotkey(self, hotkey_name):
        try:
            vk_code = None

            # F1-F12
            if hotkey_name.upper().startswith('F') and hotkey_name[1:].isdigit():
                vk_code = 111 + int(hotkey_name[1:])
            # Cyfry 0-9
            elif hotkey_name.isdigit():
                vk_code = ord(hotkey_name)
            # Pojedyncza litera A-Z
            elif len(hotkey_name) == 1 and hotkey_name.isalpha():
                vk_code = ord(hotkey_name.upper())
            # Spacja
            elif hotkey_name.lower() == 'space':
                vk_code = 0x20
            # Numpad *
            elif hotkey_name == '*':
                vk_code = 0x6A
            else:
                print(f"Nieznany klawisz: {hotkey_name}")
                return

            win32gui.PostMessage(Addresses.game, win32con.WM_KEYDOWN, vk_code, 0x00010001)
            win32gui.PostMessage(Addresses.game, win32con.WM_KEYUP,   vk_code, 0xC0010001)

        except Exception as e:
            print(f"Error pressing hotkey {hotkey_name}: {e}")

    def stop(self):
        self.running = False
