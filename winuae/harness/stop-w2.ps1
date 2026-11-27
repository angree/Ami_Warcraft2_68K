# Close OUR guest (matched by the FULL path of its config, D:\GITHUB\_AmiW2-poza-repo\emu\<cfg>, on the command line) with
# WM_CLOSE. Never by process name, never -Force: a killed WinUAE can corrupt a hardfile, and the other
# guests on this PC belong to other projects and to the user.
#   stop-w2.ps1 [-Config w2-ref020.uae]
param([string]$Config = "w2-ref020.uae", [int]$TimeoutSec = 40)
$OurExe = "D:\GITHUB\_AmiW2-poza-repo\emu\uae\winuae-w2.exe"   # the ONLY executable these scripts may touch: exact ExecutablePath
# AND D:\GITHUB\_AmiW2-poza-repo\emu + our config in the command line. Never by name, window title or "winuae" match.
$cfg = Join-Path "D:\GITHUB\_AmiW2-poza-repo\emu" $Config
$mine = @(Get-CimInstance Win32_Process -Filter "Name = 'winuae-w2.exe'" | Where-Object { $_.ExecutablePath -eq $OurExe -and $_.CommandLine -like '*D:\GITHUB\_AmiW2-poza-repo\emu*' -and $_.CommandLine -like "*$cfg*" })
if ($mine.Count -eq 0) { Write-Output "not running"; exit 0 }
# a clean end of the guest-side job queue first (jobrun polls Work:jobs/quit)
$workdir = ((Get-Content $cfg -ErrorAction SilentlyContinue | Where-Object { $_ -match '^filesystem2=.*WORK:Work:' } | Select-Object -First 1) -replace '^.*WORK:Work:', '' -replace ',-?[0-9]+$', '')
if ($workdir -and (Test-Path (Join-Path $workdir "jobs"))) { Set-Content (Join-Path $workdir "jobs\quit") "quit"; Start-Sleep -Seconds 2 }
foreach ($m in $mine) {
  $p = Get-Process -Id $m.ProcessId -ErrorAction SilentlyContinue
  if ($p -and $p.Path -eq $OurExe -and $p.MainWindowHandle -ne 0) { [void]$p.CloseMainWindow(); Write-Output "WM_CLOSE -> pid $($p.Id) $($p.Path) [$($m.CommandLine)]" }
}
$deadline = (Get-Date).AddSeconds($TimeoutSec)
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Milliseconds 500
  $left = @(Get-CimInstance Win32_Process -Filter "Name = 'winuae-w2.exe'" | Where-Object { $_.ExecutablePath -eq $OurExe -and $_.CommandLine -like '*D:\GITHUB\_AmiW2-poza-repo\emu*' -and $_.CommandLine -like "*$cfg*" })
  if ($left.Count -eq 0) {
    if ($workdir) { Remove-Item (Join-Path $workdir "jobs\quit") -ErrorAction SilentlyContinue }
    Write-Output "stopped"; exit 0 }
}
Write-Output "STILL RUNNING after $TimeoutSec s - not forcing it; ask the user"
exit 1
