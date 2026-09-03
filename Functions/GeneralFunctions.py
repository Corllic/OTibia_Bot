import io
import urllib

import requests
import win32con
import win32gui
import win32ui
from PIL import Image, ImageSequence, ImageFile
import numpy as np
import cv2 as cv
from PyQt5.QtCore import Qt
from bs4 import BeautifulSoup

import Addresses
import os
import json


def load_items_images(list_widget) -> None:
    zoom_img = 3
    Addresses.item_list = {}
    for item_index in range(list_widget.count()):
        item_name = list_widget.item(item_index).text()
        item_data = list_widget.item(item_index).data(Qt.UserRole)
        loot_container = item_data['Loot']
        item = Image.open(f'Images/{Addresses.client_name}/{item_name}.png').convert('RGBA')
        item = np.array(item)
        item = item[:22, :, :]
        item = cv.cvtColor(item, cv.COLOR_BGR2GRAY)
        item = cv.GaussianBlur(item, (7, 7), 0)
        item = cv.resize(item, None, fx=zoom_img, fy=zoom_img, interpolation=cv.INTER_CUBIC)
        Addresses.item_list[item_name] = []
        Addresses.item_list[item_name].append(item)
        Addresses.item_list[item_name].append(loot_container)


def merge_close_points(points, distance_threshold):
    merged_points = []
    merged_indices = set()

    def merge_distance(point1, point2):
        return np.sqrt(np.sum((point1 - point2) ** 2))
    for i in range(len(points)):
        if i not in merged_indices:
            current_point = points[i]
            merged_point = np.array(current_point)
            for j in range(i + 1, len(points)):
                if merge_distance(np.array(current_point), np.array(points[j])) < distance_threshold:
                    merged_point = (merged_point + np.array(points[j])) / 2
                    merged_indices.add(j)
            merged_points.append(tuple(merged_point))
    return merged_points


class WindowCapture:
    def __init__(self, w, h, x, y):
        self.hwnd = Addresses.game  # uzyj bezposrednio HWND zamiast szukac po tytule
        self.w = w
        self.h = h
        self.x = x
        self.y = y

    def get_screenshot(self):
        try:
            import ctypes
            hwnd = self.hwnd
            if not hwnd:
                return None

            # Uzyj PrintWindow z PW_RENDERFULLCONTENT (działa z OpenGL/DX)
            wDC = win32gui.GetWindowDC(hwnd)
            dc_obj = win32ui.CreateDCFromHandle(wDC)
            cDC = dc_obj.CreateCompatibleDC()
            data_bitmap = win32ui.CreateBitmap()
            # Pobierz caly rozmiar okna
            rect = win32gui.GetWindowRect(hwnd)
            full_w = rect[2] - rect[0]
            full_h = rect[3] - rect[1]
            if full_w <= 0 or full_h <= 0:
                dc_obj.DeleteDC(); cDC.DeleteDC()
                win32gui.ReleaseDC(hwnd, wDC); return None

            data_bitmap.CreateCompatibleBitmap(dc_obj, full_w, full_h)
            cDC.SelectObject(data_bitmap)

            # PrintWindow z PW_RENDERFULLCONTENT = 0x2 przechwytuje OpenGL
            PW_RENDERFULLCONTENT = 0x00000002
            result = ctypes.windll.user32.PrintWindow(hwnd, cDC.GetSafeHdc(), PW_RENDERFULLCONTENT)

            signed_ints_array = data_bitmap.GetBitmapBits(True)
            img = np.frombuffer(signed_ints_array, dtype='uint8')
            img.shape = (full_h, full_w, 4)

            dc_obj.DeleteDC()
            cDC.DeleteDC()
            win32gui.ReleaseDC(hwnd, wDC)
            win32gui.DeleteObject(data_bitmap.GetHandle())

            img = img[..., :3]
            img = np.ascontiguousarray(img)

            # Przytnij do zadanego obszaru
            y1 = min(self.y, full_h)
            y2 = min(self.y + self.h, full_h)
            x1 = min(self.x, full_w)
            x2 = min(self.x + self.w, full_w)
            if y2 <= y1 or x2 <= x1:
                return img
            return img[y1:y2, x1:x2]

        except Exception as e:
            return None


def delete_item(list_widget, item) -> None:
    index = list_widget.row(item)
    list_widget.takeItem(index)


def manage_profile(action: str, directory: str, profile_name: str, data: dict = None):
    file_path = os.path.join(directory, f"{profile_name}.json")
    if action.lower() == "save":
        if not os.path.exists(directory):
            os.makedirs(directory)
        with open(file_path, "w") as f:
            json.dump(data, f, indent=4)
        return True
    elif action.lower() == "load":
        if not os.path.exists(file_path):
            return {}
        with open(file_path, "r") as f:
            return json.load(f)




