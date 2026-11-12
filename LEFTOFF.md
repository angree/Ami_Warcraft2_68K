# LEFTOFF — hand-off (read first)

## EHB / ECS (2026-11-13) - grafika 64-kolorowa ZROBIONA na hoście, do sprawdzenia w emulatorze

- `backend=ehb` (zawsze 320x240): grafika konwertowana z góry do 64 kolorów EHB. Nowe `src/ehb.c/.h`
  (paleta: 32 rejestry 12-bit + sprzętowe połówki; k-means na liczbach całkowitych), `src/lores.c`
  (LoresQ z paletą docelową != źródłowa; paczki `<data_dir>/ehb-<era>.pack` = format lores + na końcu
  "EHBP" + 64xRGB), `src/assets.c` (`assets_set_ehb/assets_ehb`, wirtualna paleta 256 wpisów = E[i&63],
  `assets_src_map()` dla surowej grafiki ui/menu, `assets_load_palette_for()` z listą grafik pod wagi).
- Rejestry: 0 czarny; gracze: czerwony 1, niebieski 2, zielony 3, fioletowy 4, pomarańczowy 5, żółty 6,
  czarny 17, biały 19; 18 = szary 152 (17/18/19 = kolory wskaźnika na OCS/ECS); reszta 22 z k-means.
  Rampa gracza 4 odcienie -> rejestr (2 jasne) + połówka (2 ciemne). W sprite'ach kolor gracza jest
  zapisany jako para 4/36 (rejestr fioletowego), remap zamienia ją na rejestr gracza.
- `w2conv` buduje paczki EHB zaraz po lores (`--no-ehb` pomija); bajt w bajt jak gra. Paczki skopiowane
  do gamedata/data i C:/temp/amiga_w2/work{,-play,-aros}/data.
- Host emuluje EHB jak chipset (`native/host_gfx.c`): 12 bitów, połówki, licznik błędów
  (`EHB check: ... (clean)`): piksel >= 64 albo zapis rejestru >= 32 = błąd. Skrypty i zrzuty:
  `D:\GITHUB\_AmiW2-poza-repo\scratch\ehb\` (`final.sh`, `cmp_*.png` = AGA lores | EHB).
- `native/amiga_gfx.c`: `amigagfx_set_palette` na ekranie EHB obcina do rejestrów 0..31.
- Film logo (Smacker) w EHB pominięty (paleta na klatkę). Wskaźnik w EHB: 3 kolory = rejestry 17..19
  (ciemny/szary/jasny), `ui_cursor.c` nie woła tam `amigagfx_pointer_colours`.
- NIE sprawdzone w emulatorze (robi orkiestrator): w2.cfg `backend=ehb`, `resolution=lores`.
- Ograniczenia: bez ditheringu (śnieg/pergamin menu = płaskie plamy), 12 bitów koloru; palety obrazków
  menu liczone przy pierwszym wejściu na ekran (ok. 50 przebiegów 256x64 odległości - na 030 chwila).
- Przy okazji: `tools/build_test_menu.sh` nie linkował się (brak lores.c/snd.c na liście) - naprawione;
  `tools/test_ui.c` oczekiwał kursora <= 16 px (ui_cursor.c ma PW 32 od zmiany 1:1) - poprawione.

## ZACZNIJ TUTAJ (2026-11-12 wieczór)

Runda 1 ZROBIONA: formaty (`docs/DATA_FORMAT.md`, `ENTRY_MAP.md`, `src/war.c`, `src/w2gfx_decode.c`),
tabele (`src/w2_tables.*`, `docs/SPEC_RULES.md`, generator `tools/gen_tables/`), platforma (`native/`,
`build/build.sh`, `w2setup`, WinUAE `C:\temp\amiga_w2` — AmigaOS AGA/EHB/RTG + AROS bootują),
host harness (`build/build-host.sh`), metody AmiSC (`docs/AMISC_METHODS.md`, `src/font35.*`, `src/smk.*`).
Dekompilacja Mac 68k (agent, `docs/RE_NOTES.md`) — w toku.

Wspólne (orkiestrator): `src/w2.h` (kontrakt + API modułów), `src/assets.*`, `src/pud.*`, `src/rng.c`,
`src/w2_stubs.c` (weak), `docs/TEAM_BRIEF.md`.

Runda 2 W TOKU — 9 agentów, każdy w piaskownicy `D:\GITHUB\_AmiW2-poza-repo\wt\<moduł>`, na końcu kopiują
swoje pliki do repo: game+mapdraw, unit+unitdraw, path+move, combat+missile, economy, ui+cursor, ai,
snd+w2conv (konwerter Mac CD/PC + muzyka z CD audio), menu.

INTEGRACJA (2026-11-12 noc) - pierwsza grywalna wersja:
- host: wszystkie moduły razem (bez lores), `scratch/integ/run_aivai.sh 220 1800000` (cfg `spectate=1`
  = AI gra też slot lokalny): 30 min gry, fale ataków od ~c17000, 4 z 6 graczy pokonanych, bez ASan.
- Amiga: build/build.sh OK (w2 285 KB), WinUAE w2-ref020 RTG: autostart 220 działa, panel UI, mgła,
  budowa; ~9-13 fps na emulowanym 020 (do optymalizacji: dirty rects, frameskip logiki).
- dane gracza do testów: C:/temp/amiga_w2/work/data (kopia lokalna, nie repo), cfg data_dir=PROGDIR:data.
- poprawione przy integracji: OR w tabeli zależności, klasyfikacja drzewo/skała po numerze kafla
  (obie grupy mają flagę 0x81), złoto kopalni 0 -> 100000, AI ulepsza właściwy ratusz.
- lores: agent skończył (lores.c, assets.c; paczki lores-<era>.pack w data_dir, ~2 min budowy na 030 przy pierwszym starcie - brak paska postępu na ekranie).
- AmigaOS AGA lores 320x240: działa (~15 fps emul. 020, c2p); AROS 68k RTG hires: działa (~22 fps). Kolory wskaźnika na AROS złe (pens 17-19 = czerwienie) - do poprawy. MorphOS: jeszcze nie testowany.
- NASTĘPNE: MorphOS (QEMU, tylko nasza instancja), wydajność (dirty rects w game_draw, frameskip logiki), muzyka (w2conv z CD audio), czary/statki/transport, kampanie.
- 2026-11-12 23:10: AGA hires czarny ekran naprawiony (game_draw nie ustawiał view.buf dla menu); menu ma flagę dirty (50 fps zamiast 2.8 w AGA); kursor gry także w menu, 1:1 w hires (sprite 70 ns), paleta kursora = War Data 2; tytuł W2_NAME "Warcraft II 68K"; w2setup: "Amiga bar (multitask)" + krótsze etykiety (okno mieści się w 640). WinUAE: WSZYSTKIE testy na max speed -70% bez JIT (też w2-play).
- 23:45: lores - podgląd budynku przesunięty (ui.c mnożył hires box_w razy 2, a lores agent zmienił box na zawsze hires) - naprawione, sprawdzone w emulatorze. Dźwięk: snd_service w każdym stanie, muzyka menu/victory/defeat, music/ skopiowane do work*/data. test-w2.ps1 ma -Actions/-ShotMs. Ekstraktor PC GOTOWY: w2conv.exe (Windows, statyczny) robi data/ + music/ + paczki lores (bajt w bajt jak gra), build/package-w2conv.sh -> _AmiW2-poza-repo/dist/w2conv/ (exe + extract.bat + README.txt). Nietestowane: PC CD / folder jako źródło.

Dawny plan: integracja (wszystkie moduły razem na hoście z autostart=220, AI vs AI), potem lores
(agent: lores.c + przejęcie assets.c), potem build Amiga + test WinUAE (`winuae\harness\test-w2.ps1`).

Do zrobienia przy integracji (zebrane z raportów):
- `AmigaAudio_StopChannel(ch)` w native/amiga_audio (snd chce kraść kanały); host build bez native/amiga_adpcm.c.
- wywołania snd: menu `snd_music_play(SND_MUS_MENU,0)`, endscreen victory/defeat, game.c snd_init/service.
- `menu_wants_quit()` -> game_wants_quit; `assets_restore_era_palette()` (lores agent dodaje).
- minimapa: game.c rysuje wnętrze (24,26), ui.c tylko ramka (`game_draws_minimap`).
- corpses: wyczyść przy nowej mapie (unit.c czyści przy pierwszym unit_create w cyklu 0).
- menu: game_wants_quit -> menu_wants_quit(); GS_MENU i GS_END -> menu_draw/menu_event; game_end(3) -> endscreen_enter(3); typ gry przez menu_game_type().
- listing katalogu maps/: ZROBIONE (`w2_dir_list` w native/w2_sys.c Lock/ExNext + host_sys.c; menu.c używa).
- move: unit.c woła `move_tick(slot)` co cykl dla każdej jednostki; `move_release(slot)` przy śmierci/unit_remove/wejściu do kopalni/transportu; game.c `move_reset_all()` po wczytaniu mapy.
- combat: `ord_still`/`ord_stand` -> `t=combat_acquire(s); if(t>=0) combat_engage(s,t);` (stand ground: atak tylko w zasięgu - funkcja po stronie combat); bloodlust: W2Unit.damage_bonus/armor_bonus; upgrade'y = bit n W2Player.upgrades, econ_stat() ma pierwszeństwo; ogień budynków `combat_burn_level` -> unitdraw.c.
- ai: tabela zależności OR - ZROBIONE (gen.py czyta varargs "or", w2_tables.c ma alt 0/1); cmd_harvest(target=-1) = drzewo; W2Player.upgrades 32 bity na 52 upgrade'y (AI używa <32) - do poszerzenia.
- tryb gry: takt z oryginału (main_amiga.c w2_tick_ms), RNG z oryginału (rng.c), wzór obrażeń w RE_NOTES.

EMULATORY: tylko `C:\temp\amiga_w2\uae\winuae-w2.exe`; nigdy nie zamykać innych (użytkownik!).

## WYDANIE 0.1.0 (2026-11-13)
- github.com/angree/Ami_Warcraft2_68K, tag v0.1.0: `Warcraft2_68K-0.1.0.lha` + `Warcraft2_68K-0.1.0-converter.zip` (`build/release.sh` przez `build/w.sh`; bramka `build/release-gate.py`; paczki w `_AmiW2-poza-repo\dist\release-0.1.0`). Licencja GPL v2.
- EHB (ECS) działa: `src/ehb.c`, paczki `ehb-<era>.pack` z w2conv, 8 rejestrów na kolory graczy; sprawdzone w WinUAE (-70%, bez JIT) grą i menu. Zrzut z gościa (`snap`) pokazuje teraz połówki EHB poprawnie.
- Menu czyta też mapy z `<data_dir>/pud` (mapy z płyty).
- ZALEGŁOŚCI: testy modułów unit/move/combat/economy/ai/snd nie linkują się od integracji (stare listy plików, własne atrapy ruchu) - naprawić skrypty `tools/build_test_*.sh`; MorphOS nietestowany; kursor w EHB nieoglądany na ekranie (zrzuty okna hosta wychodziły czarne); brak ditheringu w EHB; wydajność AGA hires; kampanie, czary, transport, zapis gry.
