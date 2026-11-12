@echo off
rem Warcraft II Mac 68k -> Ghidra headless analysis + full decompilation.
rem Everything is written to OUT (outside the repository).  Prerequisites:
rem   1. python tools\re\mac68k_link.py "hfs\Warcraft II.rsrc" %OUT%
rem   2. mac68020.slaspec + mac68k.ldefs copied into Ghidra\Processors\68000\data\languages and
rem      compiled once: support\sleigh.bat <full canonical path>\mac68020.slaspec
rem usage: run_w2.bat [import|export]   (import = fresh project + analysis + export)
setlocal
if "%GHIDRA%"=="" set GHIDRA=C:\Tools\ghidra_12.0.4_PUBLIC
if "%OUT%"=="" set OUT=D:\GITHUB\_AmiW2-poza-repo\re68k
set JAVA_HOME=C:\Program Files\Microsoft\jdk-21.0.10.7-hotspot
set GHIDRA_HEADLESS_MAXMEM=8G
set SCRIPTS=%~dp0
set SCRIPTS=%SCRIPTS:~0,-1%
mkdir %OUT%\proj 2>nul
if /i "%1"=="export" goto export
call %GHIDRA%\support\analyzeHeadless.bat %OUT%\proj w2 -import %OUT%\w2_image.bin -overwrite ^
  -loader BinaryLoader -loader-baseAddr 0x100000 -processor 68000:BE:32:MacTraps -cspec default ^
  -analysisTimeoutPerFile 86400 -scriptPath %SCRIPTS% ^
  -preScript W2Import.java %OUT% -postScript W2Export.java %OUT% %OUT%\w2_all.c ^
  -log %OUT%\analyze.log > %OUT%\run.out 2>&1
goto end
:export
call %GHIDRA%\support\analyzeHeadless.bat %OUT%\proj w2 -process w2_image.bin -noanalysis ^
  -scriptPath %SCRIPTS% -postScript W2Export.java %OUT% %OUT%\w2_all.c ^
  -log %OUT%\export.log > %OUT%\run_export.out 2>&1
:end
echo done %errorlevel%
