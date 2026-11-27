# Building Warcraft II 68K on Windows without WSL

Everything needed for the Amiga binaries, the release packages and the tests in the emulator runs
natively on Windows; the native MorphOS PPC binary is built by GitHub Actions. Versions 0.6.2 and 0.6.3
were built this way, while WSL was unavailable.

Short version of a full release:

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\build\build-win.ps1
git push                                   (GitHub Actions builds w2_MorphOS - section 5)
gh run download <id> -R angree/Ami_Warcraft2_68K -n w2-morphos-ppc -D D:\GITHUB\_AmiW2-poza-repo\mos
py -I D:\GITHUB\Ami_Warcraft2\tools\icons\make_mos_icon.py D:\GITHUB\_AmiW2-poza-repo\wbuild\icons\w2.png D:\GITHUB\_AmiW2-poza-repo\mos\w2_MorphOS.info
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\build\release-win.ps1 -MorphOS D:\GITHUB\_AmiW2-poza-repo\mos -ConverterFrom <previous ...-converter.zip>
gh release create v<version> ...           (section 3)
```

## 1. What is needed

| Tool | Where | Used for |
|---|---|---|
| bebbo amiga-gcc **6.5.0b for Windows** | `C:\amiga-gcc` (installer from bebbo's amiga-gcc releases; it brings `m68k-amigaos-gcc.exe`, `vasmm68k_mot.exe`, `lha.exe`, libnix, the NDK) | compiling and linking `w2`, `w2setup`, `jobrun`, the c2p assembler, LHA archives |
| Python 3 (`py` launcher) with Pillow | Windows install | Workbench icons (`tools/icons/draw_icons.py`, `tools/mkicon_newicons.py`), release gate (`build/release-gate.py`) |
| PowerShell 5.1 | part of Windows | the build scripts |
| `gh` (GitHub CLI), logged in | Windows install | publishing a release |
| WinUAE runtime of this project | `D:\GITHUB\_AmiW2-poza-repo\emu` (`uae\winuae-w2.exe`, configs, boot disks, `work*` folders) | tests in the emulator |

It is the same compiler (gcc 6.5.0b, libnix) and the same flags as the WSL build in `build/build.sh`:
`-mcpu=68020 -msoft-float -noixemul -O1 -fomit-frame-pointer -fbbb=abcfhlmnprsz0 -std=gnu99`, never
`-m68040`/`-lc`/`-lpthread`, never strip, `fp_*.c` with `-fno-builtin` linked before `-lm`; the build
refuses a link that pulls in `libm881` and FPU mnemonics in the `.s` files.

## 2. The game: `w2`, `w2setup` (+ `jobrun` for the test harness)

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\build\build-win.ps1
```

- Objects and binaries go to `D:\GITHUB\_AmiW2-poza-repo\wbuild` (outside the repository).
- Deployed to the emulator folders `D:\GITHUB\_AmiW2-poza-repo\emu\work` (test guest), `work-aros` and `work-play`
  (your play guest) with fresh icons and the `run` start script (LF line ends).
- Options: `-NoDeploy` (build only), `-Deploy <dir>`, `-DeployAros none`, `-DeployPlay none`,
  `-Strict` (warnings are errors), `-Toolchain <dir>` (another amiga-gcc folder).
- The last line must be `built and deployed`; compiler warnings are printed per file.

## 3. Release packages

```
powershell -ExecutionPolicy Bypass -File D:\GITHUB\Ami_Warcraft2\build\release-win.ps1 -MorphOS D:\GITHUB\_AmiW2-poza-repo\mos -ConverterFrom D:\GITHUB\_AmiW2-poza-repo\dist\release-0.6.3\Warcraft2_68K-0.6.3-converter.zip
```

- Takes the version from `src\version.h` and writes `D:\GITHUB\_AmiW2-poza-repo\dist\release-<version>\`:
  `Warcraft2_68K-<version>.lha` (the Amiga drawer: `w2`, `w2setup`, `w2_MorphOS`, icons, README, LICENSE,
  drawer icon) and `Warcraft2_68K-<version>-converter.zip`.
- `-MorphOS <folder>` adds `w2_MorphOS` and `w2_MorphOS.info` from that folder (the GitHub Actions build of the
  SAME version, section 5, and its icon made with `tools\icons\make_mos_icon.py`: our own `w2.png` as a 64x64
  PNG, which Ambient shows as an icon). The script checks that it is an ELF file carrying the version.
  Without `-MorphOS` the release has only the 68k binaries.
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

## 5. Native MorphOS PPC binary (`w2_MorphOS`) - GitHub Actions, no WSL

No `ppc-morphos-gcc` runs natively on Windows (the MorphOS cross toolchain, metaneutrons/morphos-cross with
GCC 15 and MorphOS SDK 3.20, is published for Linux and macOS only - see the research report
`D:\GITHUB\_AmiW2-poza-repo\reports\Kompilacja PPC bez WSL.md`). The PPC binary is therefore built on a GitHub
Linux runner; nothing is installed on this PC except `gh`.

- Workflow: `.github/workflows/morphos.yml` (runs on every push that touches `src/`, `native/`, the build script
  or the workflow, and by hand: `gh workflow run "MorphOS PPC build" -R angree/Ami_Warcraft2_68K`). It installs
  morphos-cross (cached), runs `build/build-morphos.sh` and uploads `w2_MorphOS` as the artifact `w2-morphos-ppc`.
  The MorphOS SDK is downloaded by the installer during the build and never stored in the repository.
- Watch and fetch the binary:
  ```
  gh run list -R angree/Ami_Warcraft2_68K --limit 3
  gh run watch <id> -R angree/Ami_Warcraft2_68K --exit-status
  gh run download <id> -R angree/Ami_Warcraft2_68K -n w2-morphos-ppc -D D:\GITHUB\_AmiW2-poza-repo\mos
  ```
- Flags (from the instructions `45-MorphOS-natywny-PPC-2D.md`): `-O2 -fno-strict-aliasing -fwrapv -fsigned-char
  -noixemul`. `-fsigned-char` is mandatory (char is unsigned on PPC, the simulation depends on signed char).
- MorphOS-only code: `native/mos_c2p_stubs.c` (the 68k c2p is never called on MorphOS, which always uses RTG) and
  the `#ifdef __MORPHOS__` parts of `amiga_stack.c` (crt0 `__stack`, no StackSwap), `amiga_trap.c` (no 68k
  trap frames), `amiga_uclock.c` (`TimerBase` is a `struct Library`), `amiga_gfx.c` (CGX header from the SDK).
- The same script works on any Linux machine with morphos-cross on PATH: `sh build/build-morphos.sh`.

### Testing the PPC binary: own MorphOS QEMU lab `E:\morphos_w2`

A private copy of the shared MorphOS lab (`E:\morphos_emu`, used by other projects): QEMU 11.1 copied to
`E:\morphos_w2\qemu` with the executable renamed `qemu-w2-ppc.exe`, monitor on port **4557**, own exchange folder.
MorphOS 3.20 and its boot image are read (read-only) from `E:\morphos_emu`. Never stop QEMU by name; the test
script stops only its own instance (its pid file and port).

```
copy D:\GITHUB\_AmiW2-poza-repo\mos\w2_MorphOS E:\morphos_w2\xfer\w2\w2_MorphOS
powershell -ExecutionPolicy Bypass -File E:\morphos_w2\w2-test.ps1 -MaxMin 15
```

- `E:\morphos_w2\xfer\w2\` holds the binary, the game data (`data\`, without music) and the test's `w2.cfg` /
  `autoinput.txt` (RTG 640x480, computer vs computer, `profile=1`, quits after 90 s).
- The guest script `xfer\w2test` copies everything to `RAM:` and runs the game there (a game writing its log on
  the QEMU FAT disk can kill QEMU), then copies `w2.log` back as `xfer\w2-mos.log`.
- Screenshots: `E:\morphos_w2\shots\`.
- Result 2026-11-27 (0.6.2): runs to the end, RTG 640x480, 110-140 fps in QEMU, logic 7 ms/s, clean exit.
- Unregistered MorphOS slows down after 30 minutes: keep runs short (the script boots a fresh MorphOS each time).

## 6. What still needs Linux/WSL

| Item | Why |
|---|---|
| Host build with AddressSanitizer/UBSan (`build/build-host.sh`) and the host tests (campaign, spells, transport, screenshot comparisons) | needs a native Linux gcc with sanitizers |
| The PC converter `w2conv` (`build/build-w2conv.sh`) | Linux and MinGW cross build |
| The full `build/release.sh` | uses the two above |

## 7. Where the emulator lives

The WinUAE runtime is in `D:\GITHUB\_AmiW2-poza-repo\emu` (local disk, outside the repository). It used to be in
`C:	empmiga_w2`, but `C:	emp` is disposable: the disk cleanup tool `diskguard` moved it to the NAS
(`I:	emp_from_cutomiga_w2`) twice when drive C: ran low (2026-11-23, 2026-11-25). Never keep anything that
must last under `C:	emp`. If the runtime is ever missing, the NAS copy can be copied back:

```
robocopy I:	emp_from_cutomiga_w2 D:\GITHUB\_AmiW2-poza-repo\emu /E /COPY:DAT /R:1 /W:1
```
