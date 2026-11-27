# build-win.ps1 - Amiga build of AmiW2 on Windows WITHOUT WSL, with bebbo's amiga-gcc for Windows
# (C:\amiga-gcc, the same gcc 6.5.0b as /opt/amiga in WSL). Same flags and gates as build/build.sh.
#   powershell -ExecutionPolicy Bypass -File build\build-win.ps1            build + deploy
#   ... -NoDeploy                                                           build only
#   ... -Deploy <dir> -DeployAros none -DeployPlay none                     other deploy folders
# Rules (!instrukcje 10-68k-2D.md): -mcpu=68020 -msoft-float, never -m68040/-lc/-lpthread, NEVER strip,
# -fbbb without i and e, fp_*.c with -fno-builtin linked ahead of -lm, refuse a link that pulls in libm881.
param([string]$Toolchain = "C:\amiga-gcc", [string]$Work = "D:\GITHUB\_AmiW2-poza-repo\wbuild",
      [string]$Deploy = "D:\GITHUB\_AmiW2-poza-repo\emu\work", [string]$DeployAros = "", [string]$DeployPlay = "",
      [switch]$NoDeploy, [switch]$Strict)
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$bin = Join-Path $Toolchain "bin"
$env:PATH = "$bin;$env:PATH"
$gcc = Join-Path $bin "m68k-amigaos-gcc.exe"
$vasm = Join-Path $bin "vasmm68k_mot.exe"
if (-not (Test-Path $gcc)) { throw "no m68k-amigaos-gcc in $bin" }
$obj = Join-Path $Work "obj"
New-Item -ItemType Directory -Force $obj | Out-Null
$R = $repo -replace '\\', '/'
$cpu = @("-mcpu=68020", "-msoft-float", "-noixemul")
$cflags = $cpu + @("-O1", "-fomit-frame-pointer", "-fbbb=abcfhlmnprsz0", "-Wall", "-std=gnu99", "-D__AMIGA__",
                   "-I$R/native", "-I$R/native/cgx-include", "-I$R/src")
if ($Strict) { $cflags += "-Werror" }

function Compile([string]$src, [string[]]$extra) {
	$o = Join-Path $obj (([IO.Path]::GetFileName($src)) + ".o")
	$warn = "$o.warn"
	& $gcc @cflags @extra -c $src -o $o 2> $warn
	if ($LASTEXITCODE -ne 0) { Get-Content $warn; throw "COMPILE FAILED: $src" }
	if ((Get-Item $warn).Length -gt 0) { Write-Output "--- warnings: $([IO.Path]::GetFileName($src))"; Get-Content $warn }
	return $o
}

# game sources: every src/*.c except the settings tool, w2cfg.c (listed explicitly), main_amiga.c, host/test files
$skip = @("setup_amiga.c", "w2cfg.c", "main_amiga.c")
$game = Get-ChildItem (Join-Path $repo "src") -Filter *.c | Where-Object { $skip -notcontains $_.Name -and $_.Name -notlike "*_host.c" -and $_.Name -notlike "test_*.c" }
$objs = @()
foreach ($f in "fp_conv.c", "fp_single.c", "fp_double.c") { $null = Compile "$R/native/$f" @("-fno-builtin") }
$nat = @("amiga_gfx", "amiga_trap", "amiga_stack", "amiga_uclock", "amiga_audio", "amiga_adpcm", "w2_sys", "cellcmp")
foreach ($f in $nat) { $objs += Compile "$R/native/$f.c" @() }
foreach ($f in $game) { $objs += Compile "$R/src/$($f.Name)" @() }
$cfgo = Compile "$R/src/w2cfg.c" @()
# chunky-to-planar (Kalms); FPU gate on the assembler sources
$fpu = Select-String -Path (Join-Path $repo "native\*.s") -Pattern '^\s+f(add|sub|mul|div|move|sqrt|int|cmp|mov)' -CaseSensitive:$false
if ($fpu) { throw "FPU GATE: FPU mnemonic in a .s file - refusing" }
& $vasm -quiet -Fhunk -m68020 -no-opt "-I$($Toolchain -replace '\\','/')/m68k-amigaos/ndk-include" "-I$R/native" -o (Join-Path $obj "c2p_glue.o") "$R/native/c2p_glue.s"
if ($LASTEXITCODE -ne 0) { throw "ASSEMBLY FAILED: c2p_glue.s" }
$main = Compile "$R/src/main_amiga.c" @("-DW2_HAVE_GAME")
Write-Output "game hooks: from src/ (W2_HAVE_GAME)"

$all = @($main, $cfgo) + $objs + @((Join-Path $obj "c2p_glue.o"), (Join-Path $obj "fp_conv.c.o"), (Join-Path $obj "fp_single.c.o"), (Join-Path $obj "fp_double.c.o"))
$w2 = Join-Path $Work "w2"
& $gcc @cpu "-Wl,-Map,$Work\w2.map" "-Wl,-t" -o $w2 @all -lamiga -lm > (Join-Path $Work "link.txt") 2>&1
if ($LASTEXITCODE -ne 0) { Get-Content (Join-Path $Work "link.txt"); throw "LINK FAILED w2" }
if (Select-String -Path (Join-Path $Work "link.txt") -Pattern "libm881" -Quiet) { throw "FPU GATE: libm881 was linked into w2 - refusing" }
if (Select-String -Path (Join-Path $Work "link.txt") -Pattern "duplicate section" -Quiet) { Write-Output "WARNING: duplicate section (Hunk has no COMDAT)" }

$so = Compile "$R/src/setup_amiga.c" @()
$setup = Join-Path $Work "w2setup"
& $gcc @cpu "-Wl,-t" -o $setup $so $cfgo -lamiga > (Join-Path $Work "link_setup.txt") 2>&1
if ($LASTEXITCODE -ne 0) { Get-Content (Join-Path $Work "link_setup.txt"); throw "LINK FAILED w2setup" }
if (Select-String -Path (Join-Path $Work "link_setup.txt") -Pattern "libm881" -Quiet) { throw "FPU GATE: libm881 in w2setup - refusing" }
$jo = Compile "$R/native/jobrun.c" @()
$jobrun = Join-Path $Work "jobrun"
& $gcc @cpu -o $jobrun $jo > (Join-Path $Work "link_jobrun.txt") 2>&1
if ($LASTEXITCODE -ne 0) { Get-Content (Join-Path $Work "link_jobrun.txt"); throw "LINK FAILED jobrun" }

function DeployTo([string]$d, [bool]$play) {
	New-Item -ItemType Directory -Force $d | Out-Null
	Copy-Item $w2 (Join-Path $d "w2") -Force          # NEVER strip (a stripped Hunk halts the machine)
	Copy-Item $setup (Join-Path $d "w2setup") -Force
	Copy-Item $jobrun (Join-Path $d "jobrun") -Force
	# icons: our own drawing (tools/icons) + NewIcons; kept as they are if Python/Pillow are missing
	$icons = Join-Path $Work "icons"
	& py -I (Join-Path $repo "tools\icons\draw_icons.py") $icons *> $null
	if ($LASTEXITCODE -eq 0) {
		foreach ($ic in "w2", "w2setup") {
			& py -I -W ignore (Join-Path $repo "tools\mkicon_newicons.py") (Join-Path $icons "$ic.png") (Join-Path $d "$ic.info") --tool --stack 2000000 *> $null
		}
	} elseif (-not (Test-Path (Join-Path $d "w2.info"))) { Write-Output "WARNING: icons not built (Python/Pillow) - $d has none" }
	$run = if ($play) { "; AmiW2 play guest: no job queue, Workbench comes up (double-click w2setup / w2)`nStack 2000000`n" } else { "Stack 2000000`nWork:jobrun`n" }
	[IO.File]::WriteAllText((Join-Path $d "run"), $run)       # LF only, or AmigaDOS runs nothing
}
if (-not $NoDeploy) {
	DeployTo $Deploy $false
	if ($DeployAros -eq "" -and (Test-Path "D:\GITHUB\_AmiW2-poza-repo\emu\work-aros")) { $DeployAros = "D:\GITHUB\_AmiW2-poza-repo\emu\work-aros" }
	if ($DeployPlay -eq "" -and (Test-Path "D:\GITHUB\_AmiW2-poza-repo\emu\work-play")) { $DeployPlay = "D:\GITHUB\_AmiW2-poza-repo\emu\work-play" }
	if ($DeployAros -and $DeployAros -ne "none") { DeployTo $DeployAros $false; Write-Output "also deployed to $DeployAros" }
	if ($DeployPlay -and $DeployPlay -ne "none") { DeployTo $DeployPlay $true; Write-Output "also deployed to $DeployPlay" }
}
& (Join-Path $bin "m68k-amigaos-size.exe") $w2 $setup
Get-Item $w2, $setup | Select-Object Name, Length | Format-Table -AutoSize | Out-String
Write-Output "built and deployed"
