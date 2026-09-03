"""
CatchThread - automatyczne łapanie Pokemonów
- Skanuje obszar gry po obrazkach (jak Looting) lub po nazwie (shiny/normal)
- Gdy znajdzie dopasowanie, wciska przypisany klawisz
- Nie rusza myszą - wysyła klawisze przez PostMessage
"""
import random
import time
import os
import win32gui
import win32con
import win32api
import cv2 as cv
import numpy as np

import Addresses
from PyQt5.QtCore import QThread
from Functions.GeneralFunctions import WindowCapture


def _press_key(vk_code):
    """Wysyła klawisz do okna gry bez przejmowania fokusu."""
    win32gui.PostMessage(Addresses.game, win32con.WM_KEYDOWN, vk_code, 0x00010001)
    win32gui.PostMessage(Addresses.game, win32con.WM_KEYUP,   vk_code, 0xC0010001)


def _key_str_to_vk(key_str):
    """Konwertuje string klawisza na VK code."""
    if not key_str:
        return None
    key_str = key_str.strip()
    if key_str.upper().startswith('F') and key_str[1:].isdigit():
        return 111 + int(key_str[1:])
    elif key_str.isdigit():
        return ord(key_str)
    elif len(key_str) == 1 and key_str.isalpha():
        return ord(key_str.upper())
    elif key_str == '*':
        return 0x6A
    elif key_str.lower() == 'space':
        return 0x20
    return None


class CatchThread(QThread):
    """
    Watek lapania Pokemon:
    - rules: lista slownikow:
        {
          "enabled": True,
          "type": "shiny" | "normal" | "image",
          "key": "1",           # klawisz do wcisniecia
          "cooldown": 2500,     # ms miedzy rzutami
          "image_path": "",     # sciezka do obrazka (opcjonalna)
          "threshold": 0.85,    # prog podobienstwa obrazka
        }
    - scan_area: (x, y, w, h) obszar ekranu do skanowania
                 jesli None - skanuje cale okno gry
    """

    def __init__(self, rules, scan_area=None):
        super().__init__()
        self.running = True
        self.rules = rules
        self.scan_area = scan_area
        self.last_catch_times = {}  # cooldown osobno dla kazdej reguly
        self.templates = {}  # cache zaladowanych obrazkow

    def _load_template(self, image_path):
        """Laduje i cachuje template do matchowania."""
        if image_path in self.templates:
            return self.templates[image_path]
        if not image_path or not os.path.exists(image_path):
            return None
        img = cv.imread(image_path, cv.IMREAD_COLOR)
        if img is None:
            return None
        self.templates[image_path] = img
        return img

    def _scan_for_shiny_color(self, screenshot):
        """
        Wykrywa napis 'Shiny' na podstawie bardzo specyficznego złotego koloru.
        Napis 'Shiny' w PokeAvalar to BARDZO jasny złoto-żółty (prawie biały złoty).
        Kolor pikseli tekstu: R>230, G>200, B<50 (ciemny niebieski/zielony kanał)
        ORAZ wymagamy klastrów (wiele sąsiednich pikseli) żeby uniknąć false positives.
        """
        try:
            img = cv.cvtColor(screenshot, cv.COLOR_BGR2RGB)

            # Bardzo specyficzny zakres - jasny złoty tekst nad Pokemonem
            # R bardzo wysoki, G wysoki-sredni, B niski
            lower = np.array([220, 180, 0],  dtype=np.uint8)
            upper = np.array([255, 240, 60], dtype=np.uint8)
            mask = cv.inRange(img, lower, upper)

            # Erozja i dylatacja - eliminuje pojedyncze piksele, zostawia klastry
            kernel = np.ones((2, 2), np.uint8)
            mask = cv.erode(mask, kernel, iterations=1)
            mask = cv.dilate(mask, kernel, iterations=1)

            # Liczba pikseli po oczyszczeniu - wymaga sporych klastrów tekstu
            count = cv.countNonZero(mask)

            # Wymaga minimum 80 pikseli w klastrze - eliminuje przypadkowe piksele
            return count > 80

        except Exception:
            return False

    def _scan_for_image(self, screenshot, template, threshold=0.85):
        """Szuka obrazka na screenshocie. Zwraca True jesli znaleziony."""
        try:
            result = cv.matchTemplate(screenshot, template, cv.TM_CCOEFF_NORMED)
            _, max_val, _, _ = cv.minMaxLoc(result)
            return max_val >= threshold
        except Exception:
            return False

    def _scan_battle_list_ocr(self, contains_text):
        """
        Skanuje obszar battle listy OCR pod katem tekstu.
        Zwraca True jesli znaleziono.
        """
        try:
            import pytesseract
            from pytesseract import Output

            bx = Addresses.battle_x[0]
            by = Addresses.battle_y[0]
            bw = Addresses.screen_width[1]
            bh = Addresses.screen_height[1]

            if bh <= by or bw <= bx:
                # Brak ustawionego obszaru battle listy
                return False

            capture = WindowCapture(bw - bx, bh - by, bx, by)
            screenshot = capture.get_screenshot()
            gray = cv.cvtColor(screenshot, cv.COLOR_BGR2GRAY)
            _, thresh = cv.threshold(gray, 150, 255, cv.THRESH_BINARY_INV)

            text_all = pytesseract.image_to_string(thresh).lower()
            return contains_text.lower() in text_all
        except Exception:
            return False

    def _get_screenshot(self):
        """Robi screenshot obszaru gry."""
        try:
            if self.scan_area:
                x, y, w, h = self.scan_area
            else:
                # Cale okno gry
                rect = win32gui.GetClientRect(Addresses.game)
                x, y, w, h = 0, 0, rect[2], rect[3]
                if w <= 0 or h <= 0:
                    return None

            capture = WindowCapture(w, h, x, y + Addresses.TITLE_BAR_OFFSET)
            return capture.get_screenshot()
        except Exception:
            return None

    def run(self):
        while self.running:
            try:
                now = time.time() * 1000  # ms

                for i, rule in enumerate(self.rules):
                    if not self.running:
                        break
                    if not rule.get('enabled', True):
                        continue

                    cooldown = int(rule.get('cooldown', 2500))
                    last = self.last_catch_times.get(i, 0)
                    if now - last < cooldown:
                        continue

                    rule_type = rule.get('type', 'shiny')
                    key = rule.get('key', '1')
                    vk_code = _key_str_to_vk(key)
                    if not vk_code:
                        continue

                    found = False

                    if rule_type == 'image':
                        # Detekcja obrazkowa (template matching)
                        image_path = rule.get('image_path', '')
                        threshold = float(rule.get('threshold', 0.85))
                        template = self._load_template(image_path)
                        if template is not None:
                            screenshot = self._get_screenshot()
                            if screenshot is not None:
                                found = self._scan_for_image(screenshot, template, threshold)

                    elif rule_type == 'shiny':
                        # Detekcja zlotego koloru napisu "Shiny" nad pokemonem
                        screenshot = self._get_screenshot()
                        if screenshot is not None:
                            found = self._scan_for_shiny_color(screenshot)

                    elif rule_type == 'normal':
                        # Detekcja OCR - dowolny mob ktory NIE ma zlotego napisu Shiny
                        found = self._scan_battle_list_ocr('')
                        if found:
                            screenshot = self._get_screenshot()
                            if screenshot is not None:
                                is_shiny = self._scan_for_shiny_color(screenshot)
                                found = not is_shiny

                    if found:
                        _press_key(vk_code)
                        self.last_catch_times[i] = now
                        print(f"[Catch] Wcisnięto {key} (typ: {rule_type})")
                        QThread.msleep(random.randint(200, 400))
                        break  # tylko jedna akcja na raz

                QThread.msleep(random.randint(100, 200))

            except Exception as e:
                print(f"[CatchThread] Error: {e}")
                QThread.msleep(500)

    def stop(self):
        self.running = False
