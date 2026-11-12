# Warcraft II (Mac 68k) → Amiga 68k — rozpoznanie (2026-11-12)

## Co jest w `WAR2.zip`

- `WAR2_122.BIN/CUE`: ścieżka 1 = dane (MODE1/2352, 101 846 sektorów), ścieżki 2–17 = CD audio.
- Ścieżka danych: hybryda ISO9660 + Apple Partition Map, partycja `Apple_HFS` od bloku 87243, 320135 bloków.
- Wyciąganie: `python tools/bin2iso.py <BIN> cd/track1.iso 101846`,
  `python tools/hfsdump.py cd/track1.iso 87243 320135 hfs` (pip: `machfs rsrcfork capstone`).
- Wersja: **Warcraft II Macintosh 1.23** (vers 1), 1.22 = pierwsze wydanie Mac (vers 2).

## Binarka `Warcraft II` (APPL, `W2gm`) — FAT

| część | rozmiar | zawartość |
|---|---|---|
| data fork | 657 780 B | PEF PowerPC (`Joy!peffpwpc`), importy CFM z nazwami (InterfaceLib, SoundLib, DisplayLib…) |
| resource fork `CODE` 0–23 | 391 247 B | **kod 68k**, ~131 800 instrukcji, ~2 600 funkcji (`link a6`) |
| `DATA 0` | 51 465 B | skompresowane dane inicjowane + relokacje (runtime w stylu CodeWarrior) |

- `CODE 0` ma jeden wpis tablicy skoków; `CODE 1` sam ładuje segmenty (`GetResource('CODE')`) — własny loader.
  Segmenty 2–23 mają nagłówek 12 B (`xxxx yyyy FFFF xxxx 0000 nnnn`), wywołania przez `jsr d16(a5)`.
- Brak symboli. Wyjątek: `CODE 21` = biblioteka Smacker (RAD) z nazwami MacsBug (`smkmalloc`, `readframe`…).
- FPU: praktycznie brak (~20 trafień, prawdopodobnie szum dekodowania danych).
- Toolbox: **260 różnych pułapek A-line, 1 711 miejsc wywołań**. Dużo z tego to menu/dialogi/sieć (do zaślepienia).
- Odwołania do niskiej pamięci Maca (np. `$904` CurrentA5) — do załatania przy ładowaniu.

## Dane (`Data/`) — ten sam format co PC `.WAR`

Nagłówek LE: `u32 0x19`, `u16 liczba`, `u16 id`, tablica `u32` offsetów — identyczny z PC (maindat/rezdat/…).

| plik | wpisy | id | zawartość |
|---|---|---|---|
| War Data | 438 | 1000 | grafika, PUD-y, palety |
| War Resources | 100 | 3000 | UI |
| War Sounds | 293 | 5000 | dźwięki |
| War Strings | 105 | 4000 | teksty |
| War Music | 49 | 2000 | **muzyka cyfrowa: RIFF WAV** (Mac nie potrzebuje CD audio) |
| War Movies | 15 | 6000 | **Smacker SMK2** |
| Title Song | — | — | RIFF WAV 22 050 Hz mono 16-bit |

## Wymagania oryginału (readme „Performance")

- „Low-end" = 68040 20–25 MHz lub PPC 603 60–80 MHz; ~6 MB RAM; ekran 640x480x8.
- Opcja „More Compatible Drawing" = rysowanie przez `CopyBits` zamiast bezpośrednio do ekranu → naturalny punkt
  przechwycenia obrazu. Animacja palety (5–30% kosztu) wyłączalna (`nopalette`).

## Strategie

| | A: runtime HLE + oryginalny kod 68k natywnie | B: reimplementacja clean-room (jak EU2) | C: rekompilacja statyczna 68k→C |
|---|---|---|---|
| co robimy | loader CODE/DATA, dyspozytor Line-A (`tc_TrapCode`), podzbiór Toolboxa, łatki low-mem | własny silnik w C, Ghidra jako specyfikacja | translator jak we Frontier |
| kod Blizzarda w repo | **zero** (gracz daje własną binarkę z CD) | zero | zero (generowany lokalnie) |
| wierność | 100% (to ten sam kod) | zależy od nas | 100% |
| wydajność na 040 | jak oryginalny Mac 040 + nasz szybszy obraz | do zoptymalizowania | gorsza niż natywnie (2–4x) |
| AROS 68k | prawdopodobnie (NIEPEWNE: Line-A przez `tc_TrapCode`) | tak | tak |
| MorphOS | NIEPEWNE (Line-A w emulacji 68k) | natywny PPC | natywny PPC |
| dane gracza | **Mac CD** (binarka) | Mac **lub PC/GOG** | Mac CD |
| koszt | tygodnie | miesiące (~2 600 funkcji bez nazw) | tygodnie, potem strojenie |

Odrzucone: Wargus/Stratagus (C++17 + Lua + SDL2, za ciężki na 040).

Rekomendacja: **A, potem stopniowo B w miejscach gorących** (renderer, przewijanie, ścieżki) z oryginałem jako
wyrocznią w tym samym procesie.
