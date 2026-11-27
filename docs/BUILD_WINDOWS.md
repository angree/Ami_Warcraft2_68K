# Building Warcraft II 68K on Windows without WSL

Everything needed for the Amiga binaries, the release packages and the tests in the emulator runs
natively on Windows. This is the way version 0.6.2 was built, while WSL was unavailable.

## 1. What is needed

| Tool | Where | Used for |
|---|---|---|
| bebbo amiga-gcc **6.5.0b for Windows** | `C:\amiga-gcc` (installer from bebbo's amiga-gcc releases; it brings `m68k-amigaos-gcc.exe`, `vasmm68k_mot.exe`, `lha.exe`, libnix, the NDK) | compiling and linking `w2`, `w2setup`, `jobrun`, the c2p assembler, LHA archives |
| Python 3 (`py` launcher) with Pillow | Windows install | Workbench icons (`tools/icons/draw_icons.py`, `tools/mkicon_newicons.py`), release gate (`build/release-gate.py`) |
| PowerShell 5.1 | part of Windows | the build scripts |
| `gh` (GitHub CLI), logged in | Windows install | publishing a release |
| WinUAE runtime of this project | `C:\temp\amiga_w2` (`uae\winuae-w2.exe`, configs, boot disks, `work*` folders) | tests in the emulator |

It is the same compiler (gcc 6.5.0b, libnix) and the same flags as the WSL build in `build/build.sh`:
`-mcpu=68020 -msoft-float -noixemul -O1 -fomit-frame-pointer -fbbb=abcfhlmnprsz0 -std=gnu99`, never
`-m68040`/`-lc`/`-lpthread`, never strip, `fp_*.c` with `-fno-builtin` linked before `-lm`; the build
refuses a link that pulls in `libm881` and FPU mnemonics in the `.s` files.

## 2. The game: `w2`, `w2setup` (+ `jobrun` for the test harness)

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\build\build-win.ps1
```

- Objects and binaries go to `D:\GITHUB\_AmiW2-poza-repo\wbuild` (outside the repository).
- Deployed to the emulator folders `C:\temp\amiga_w2\work` (test guest), `work-aros` and `work-play`
  (your play guest) with fresh icons and the `run` start script (LF line ends).
- Options: `-NoDeploy` (build only), `-Deploy <dir>`, `-DeployAros none`, `-DeployPlay none`,
  `-Strict` (warnings are errors), `-Toolchain <dir>` (another amiga-gcc folder).
- The last line must be `built and deployed`; compiler warnings are printed per file.

## 3. Release packages

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\build\release-win.ps1 -ConverterFrom D:\GITHUB\_AmiW2-poza-repo\dist\release-0.6.2\Warcraft2_68K-0.6.2-converter.zip
```

- Takes the version from `src\version.h` and writes `D:\GITHUB\_AmiW2-poza-repo\dist\release-<version>\`:
  `Warcraft2_68K-<version>.lha` (the Amiga drawer: `w2`, `w2setup`, icons, README, LICENSE, drawer icon)
  and `Warcraft2_68K-<version>-converter.zip`.
- Checks that `w2` carries the name and the version, runs the **release gate** (nothing original from
  Blizzard and nothing derived from it may be inside) and unpacks the LHA again to compare every file.
- **The PC converter** (`w2conv.exe`, `w2conv-linux`) is not built here (that build needs Linux/WSL):
  `-ConverterFrom` takes it from the converter zip of an earlier release. That is only right when the
  converter sources did not change since that release; check with
  `git diff --stat <commit of that release> -- tools/w2conv src/lores.c src/ehb.c src/war.c src/w2gfx_decode.c`
  (empty output = unchanged).

Publishing (after a commit and `git push`):

```
gh release create v<version> -R angree/Ami_Warcraft2_68K --target main --title "Warcraft II 68K <version>" --notes-file <notes.md> D:\GITHUB\_AmiW2-poza-repo\dist\release-<version>\Warcraft2_68K-<version>.lha D:\GITHUB\_AmiW2-poza-repo\dist\release-<version>\Warcraft2_68K-<version>-converter.zip
```

## 4. Tests in the emulator

Start only OUR test guest (never close other emulators; see CLAUDE.md):

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\winuae\harness\start-w2.ps1 -Config w2-ref020.uae
```

One game run with settings and scripted input, log printed and a screenshot taken:

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\winuae\harness\test-w2.ps1 -Tag mytest -CfgLines "backend=aga,resolution=lores,autostart_campaign=h3,profile=1" -ShotMs 40000 -QuitMs 60000
```

(`profile=1` logs frames per second and where the time goes; `autostart=220` starts a skirmish map,
`autostart_campaign=h3` / `o9` a campaign mission, `spectate=1` lets the computer play the local side.)

Any AmigaDOS command inside the guest:

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\winuae\harness\job-w2.ps1 -Script "Stack 2000000`nWork:w2"
```

### A test program on the Amiga instead of the host

Without WSL there is no host build with sanitizers, but a test program can be compiled for the Amiga
together with the game's objects and run in the guest. Example (the oil platform test of 0.6.2):
`D:\GITHUB\_AmiW2-poza-repo\scratch\plat\build_amiga_test.ps1` with
`D:\GITHUB\_AmiW2-poza-repo\scratch\plat\test_platform_amiga.c`. Rules:

- the test's entry point is `int w2_main(void)` — `native/amiga_stack.c` owns `main` and switches to a
  2 MB stack before calling it;
- link it with every object of `wbuild\obj` except `main_amiga.c.o`, `setup_amiga.c.o`, `jobrun.c.o`,
  plus `-lamiga -lm`;
- print with `printf` + `fflush(stdout)`; the job queue returns the output;
- data path `PROGDIR:data`.

## 5. What still needs Linux/WSL

| Item | Why |
|---|---|
| Host build with AddressSanitizer/UBSan (`build/build-host.sh`) and the host tests (campaign, spells, transport, screenshot comparisons) | needs a native Linux gcc with sanitizers |
| The PC converter `w2conv` (`build/build-w2conv.sh`) | Linux and MinGW cross build |
| The full `build/release.sh` | uses the two above |

## 6. If the emulator folder is missing

`diskguard` moves folders from `C:\temp` to the NAS (`I:\temp_from_c\auto\...`) when drive C: runs
low. If `C:\temp\amiga_w2\uae\winuae-w2.exe` is missing, look in `I:\temp_from_c\auto\amiga_w2` and
copy it back (a directory junction to the NAS does not work, I: is a network drive):

```
robocopy I:\temp_from_c\auto\amiga_w2 C:\temp\amiga_w2 /E /COPY:DAT /R:1 /W:1
```
