@echo off
title EasyBot - PokeAvalar
cd /d "%~dp0"

:: Sprawdź czy Python jest zainstalowany
python --version >nul 2>&1
if errorlevel 1 (
    echo BLAD: Python nie jest zainstalowany!
    echo Pobierz Python 3.10+ z https://www.python.org/
    pause
    exit /b 1
)

:: Sprawdź czy Tesseract jest zainstalowany
if not exist "C:\Program Files\Tesseract-OCR\tesseract.exe" (
    echo UWAGA: Tesseract OCR nie jest zainstalowany.
    echo Pobierz z: https://github.com/UB-Mannheim/tesseract/wiki
    echo Bot uruchomi sie, ale OCR Battle List nie bedzie dzialac.
    echo.
)

:: Zainstaluj zależności jeśli brak
python -c "import PyQt5" >nul 2>&1
if errorlevel 1 (
    echo Instalowanie zaleznosci...
    pip install -r requirements.txt
)

:: Uruchom bota z uprawnieniami administratora (wymagane do odczytu pamięci)
echo Uruchamianie EasyBot dla PokeAvalar...
echo.
echo WAZNE: Upewnij sie ze klient PokeAvalar jest uruchomiony!
echo.
python StartPokeAvalar.py

if errorlevel 1 (
    echo.
    echo Bot zakonczyl sie z bledem.
    pause
)
