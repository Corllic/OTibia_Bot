# EasyBot dla PokeAvalar

## Jak uruchomić

1. **Uruchom klienta PokeAvalar** (zaloguj się do gry)
2. **Kliknij dwukrotnie** `START_POKEAVALAR.bat` (jako Administrator)
3. Bot **automatycznie** wykryje i połączy się z klientem

---

## Konfiguracja adresów pamięci (WYMAGANE)

Bot czyta dane HP/MP/pozycję bezpośrednio z pamięci RAM gry.
Musisz znaleźć adresy używając **Cheat Engine**.

### Jak znaleźć adresy:

1. Pobierz [Cheat Engine](https://www.cheatengine.org/)
2. Otwórz klienta PokeAvalar
3. W Cheat Engine: `Process` → wybierz `Poke Avalar (DX9).exe`
4. Szukaj wartości HP swojej postaci (typ: `2 Bytes` / Short)
5. Zmień HP (wejdź w walkę, dostań obrażenia) i kliknij "Next Scan"
6. Powtarzaj aż zostanie 1-2 adresy → to adres HP
7. Kliknij prawym na adres → `Find out what writes to this address` → znajdź base + offset

### Edytuj plik `Save/Settings/addresses.json`:

```json
{
    "game_config": {
        "square_size": "64",
        "architecture": "32"
    },
    "my_hp": {
        "address": "TUTAJ_ADRES_HEX",
        "offset": "TUTAJ_OFFSET_HEX",
        "type": "Short"
    },
    "my_hp_max": {
        "address": "",
        "offset": "TUTAJ_OFFSET_MAX_HP",
        "type": "Short"
    },
    "my_mp": {
        "address": "",
        "offset": "TUTAJ_OFFSET_MP",
        "type": "Short"
    },
    "my_x": {
        "address": "TUTAJ_ADRES_X",
        "offset": "TUTAJ_OFFSET_X",
        "type": "Int"
    }
}
```

---

## Konfiguracja Healingu

Edytuj `Save/HealingAttack/pokeavalar_default.json`:

```json
[
    {
        "Type": "HP - Spell",
        "Key": "F1",
        "Below": 70,
        "Above": 0,
        "MinMp": 0
    }
]
```

- `Key` = klawisz F1-F12 przypisany do czaru leczącego
- `Below` = lecz gdy HP spada poniżej tej wartości (%)
- `Above` = nie lecz gdy HP jest wyższe niż ta wartość (%)
- `MinMp` = minimalne MP wymagane do użycia czaru

---

## Konfiguracja Targetowania Pokemonów

Edytuj `Save/Targeting/pokeavalar_default.json`:

```json
[
    {
        "Name": "*",
        "HpFrom": "100",
        "HpTo": "0",
        "Key": "F5",
        "MinMp": "0",
        "MinHp": "0"
    }
]
```

- `Name` = nazwa Pokemona (lub `*` dla wszystkich)
- `Key` = klawisz ataku (F1-F12)
- `HpFrom`/`HpTo` = zakres HP celu w %

---

## Wymagane programy

- Python 3.10+
- [Tesseract OCR](https://github.com/UB-Mannheim/tesseract/wiki) (dla OCR Battle List)
- [Cheat Engine](https://www.cheatengine.org/) (do znalezienia adresów)
