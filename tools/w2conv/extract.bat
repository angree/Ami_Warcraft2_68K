@echo off
rem Warcraft II 68K - make the Amiga data folder from YOUR OWN Warcraft II CD (Mac or PC).
rem
rem   Drop the CD image (.cue - its .bin must be next to it - or .iso), or a folder with the game
rem   files (War Data ... or MAINDAT.WAR ...), onto this file. The result is the folder "data"
rem   next to this file: copy it into the Warcraft2_68K drawer on the Amiga (Warcraft2_68K/data).
rem   Music comes from the CD audio tracks, so use a .cue/.bin image to get it.
rem
rem Nothing is downloaded and nothing is sent anywhere; w2conv.exe only reads what you give it.
setlocal
title Warcraft II 68K data extractor
set "HERE=%~dp0"
set "CONV=%HERE%w2conv.exe"
if not exist "%CONV%" goto :noconv
set "SRC=%~1"
if "%SRC%"=="" goto :nosrc
if not exist "%SRC%" goto :nofile
echo Warcraft II 68K data extractor
echo   source: "%SRC%"
echo   output: "%HERE%data"
if exist "%HERE%data\" echo   (the folder "data" already exists - its files will be written over)
echo.
"%CONV%" "%SRC%" "%HERE%."
if errorlevel 1 goto :failed
echo.
echo Done. Copy the folder "%HERE%data"
echo to the Amiga, into the Warcraft2_68K drawer, as "data".
goto :end

:noconv
echo w2conv.exe must be in the same folder as extract.bat.
goto :end
:nosrc
echo Drag and drop your Warcraft II CD image (.cue or .iso) or game folder onto extract.bat.
goto :end
:nofile
echo Cannot find "%SRC%".
goto :end
:failed
echo.
echo The conversion FAILED - see the messages above.
:end
echo.
pause
endlocal
