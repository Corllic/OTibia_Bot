"""
Skrypt do automatycznego znalezienia adresow pamieci PokeAvalar.
Uruchom jako Administrator!

Szuka: HP, MaxHP, MP, MaxMP, X, Y, Z postaci
"""
import sys, os, ctypes as c, struct, time

sys.path.insert(0, r'd:\APP_Projekty_APP\repos\Do testow\pkgs')

import psutil
import win32process, win32gui, win32api, win32con, win32security

# === KONFIGURACJA ===
# Zmien te wartosci na aktualne statsy swojej postaci PRZED uruchomieniem!
# Sprawdz w grze ile masz HP/MP i wpisz ponizej
CURRENT_HP     = 0   # <- WPISZ AKTUALNE HP
CURRENT_MAX_HP = 0   # <- WPISZ MAX HP
CURRENT_MP     = 0   # <- WPISZ AKTUALNE MP
# ====================

def enable_debug():
    try:
        hToken = win32security.OpenProcessToken(
            win32api.GetCurrentProcess(),
            win32con.TOKEN_ADJUST_PRIVILEGES | win32con.TOKEN_QUERY
        )
        priv_id = win32security.LookupPrivilegeValue(None, win32security.SE_DEBUG_NAME)
        win32security.AdjustTokenPrivileges(hToken, False, [(priv_id, win32con.SE_PRIVILEGE_ENABLED)])
        return True
    except Exception as e:
        print(f"Debug privilege error: {e}")
        return False


def find_process():
    for p in psutil.process_iter(['pid', 'name']):
        if 'Poke Avalar' in p.info['name'] or 'DX9' in p.info['name']:
            return p
    return None


def scan_for_value(handle, value, value_type='short', base=0x400000, max_addr=0x7FFFFFFF):
    """Skanuje pamiec w poszukiwaniu wartosci."""
    fmt = {'short': ('<h', 2), 'int': ('<i', 4), 'uint': ('<I', 4)}
    pack_fmt, size = fmt.get(value_type, ('<h', 2))
    target = struct.pack(pack_fmt, value)

    found = []
    addr = base
    chunk = 65536

    class MBI(c.Structure):
        _fields_ = [
            ('BaseAddress', c.c_void_p),
            ('AllocationBase', c.c_void_p),
            ('AllocationProtect', c.c_uint32),
            ('RegionSize', c.c_size_t),
            ('State', c.c_uint32),
            ('Protect', c.c_uint32),
            ('Type', c.c_uint32),
        ]

    mbi = MBI()
    print(f"Skanowanie pamieci w poszukiwaniu wartosci {value} ({value_type})...")
    scanned = 0

    while addr < max_addr:
        if not c.windll.kernel32.VirtualQueryEx(handle, c.c_void_p(addr), c.byref(mbi), c.sizeof(mbi)):
            break

        # Tylko committed, readable memory
        if mbi.State == 0x1000 and (mbi.Protect & 0xCC):  # PAGE_READWRITE variants
            region_size = mbi.RegionSize
            if region_size > 0 and region_size < 50 * 1024 * 1024:
                buf = c.create_string_buffer(region_size)
                bytes_read = c.c_size_t()
                if c.windll.kernel32.ReadProcessMemory(handle, c.c_void_p(addr), buf, region_size, c.byref(bytes_read)):
                    data = buf.raw[:bytes_read.value]
                    idx = 0
                    while True:
                        pos = data.find(target, idx)
                        if pos == -1:
                            break
                        found.append(addr + pos)
                        idx = pos + 1
                    scanned += region_size

        addr += mbi.RegionSize

    print(f"  Przeskanowano: {scanned // 1024 // 1024}MB, znaleziono: {len(found)} trafien")
    return found


def read_value(handle, addr, value_type='short'):
    fmt = {'short': ('<h', 2), 'int': ('<i', 4), 'uint': ('<I', 4)}
    pack_fmt, size = fmt.get(value_type, ('<h', 2))
    buf = c.create_string_buffer(size)
    bytes_read = c.c_size_t()
    if c.windll.kernel32.ReadProcessMemory(handle, c.c_void_p(addr), buf, size, c.byref(bytes_read)):
        return struct.unpack(pack_fmt, buf.raw[:size])[0]
    return None


def find_static_address(handle, known_value, modules_base, value_type='short'):
    """Szuka wartosci w sekcji statycznej (base module)."""
    results = scan_for_value(handle, known_value, value_type, base=modules_base, max_addr=modules_base + 0x10000000)
    return results


def main():
    print("=" * 60)
    print("PokeAvalar Memory Scanner")
    print("=" * 60)

    enable_debug()

    proc = find_process()
    if not proc:
        print("BLAD: Klient PokeAvalar nie jest uruchomiony!")
        input("Nacisnij Enter...")
        return

    print(f"Proces: {proc.name()} PID={proc.pid}")

    handle = c.windll.kernel32.OpenProcess(0x1F0FFF, False, proc.pid)
    if not handle:
        print("BLAD: Brak uprawnien. Uruchom ten skrypt JAKO ADMINISTRATOR!")
        input("Nacisnij Enter...")
        return

    print("Handle do procesu: OK")

    # Pobierz baze modulow
    modules = win32process.EnumProcessModules(handle)
    base_address = modules[0]
    print(f"Base address: {hex(base_address)}")

    if CURRENT_HP == 0 or CURRENT_MAX_HP == 0:
        print()
        print("UWAGA: Uzupelnij CURRENT_HP i CURRENT_MAX_HP na gorze pliku!")
        print("Sprawdz w grze swoje aktualne HP/MaxHP i wpisz ponizej.")
        print()
        try:
            hp = int(input("Podaj aktualne HP: "))
            max_hp = int(input("Podaj max HP: "))
            mp = int(input("Podaj aktualne MP: "))
        except ValueError:
            print("Bledne wartosci")
            return
    else:
        hp = CURRENT_HP
        max_hp = CURRENT_MAX_HP
        mp = CURRENT_MP

    print()
    print(f"Szukam HP={hp}, MaxHP={max_hp}, MP={mp}...")
    print()

    # Skanuj HP
    hp_addresses = scan_for_value(handle, hp, 'short')
    print(f"HP ({hp}): {len(hp_addresses)} adresow")

    # Skanuj MaxHP
    max_hp_addresses = scan_for_value(handle, max_hp, 'short')
    print(f"MaxHP ({max_hp}): {len(max_hp_addresses)} adresow")

    # Skanuj MP
    mp_addresses = scan_for_value(handle, mp, 'short')
    print(f"MP ({mp}): {len(mp_addresses)} adresow")

    print()
    print("Wyniki (pierwsze 10 kazdego):")
    print()
    print("HP adresy (wpisz do my_hp -> address w addresses.json):")
    for addr in hp_addresses[:10]:
        print(f"  {hex(addr)}  (offset od base: {hex(addr - base_address)})")

    print()
    print("MaxHP adresy (wpisz do my_hp_max -> address):")
    for addr in max_hp_addresses[:10]:
        print(f"  {hex(addr)}  (offset od base: {hex(addr - base_address)})")

    print()
    print("MP adresy (wpisz do my_mp -> address):")
    for addr in mp_addresses[:10]:
        print(f"  {hex(addr)}  (offset od base: {hex(addr - base_address)})")

    print()
    print("=" * 60)
    print("WSKAZOWKA:")
    print("Adresy ktore zaczynaja sie od tego samego prefiksu co base_address")
    print(f"({hex(base_address)}) sa najprawdopodobniej statyczne - uzyj je.")
    print()
    print("Jesli jest wiele adresow: ")
    print("  1. Zmien HP w grze (wejdz w walke)")
    print("  2. Uruchom skrypt ponownie z nowym HP")
    print("  3. Porownaj liste - zostana tylko te ktore sie pokrywaja")
    print("=" * 60)

    c.windll.kernel32.CloseHandle(handle)
    input("\nNacisnij Enter...")


if __name__ == '__main__':
    main()
