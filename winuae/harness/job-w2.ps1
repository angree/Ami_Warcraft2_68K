# Run an AmigaDOS script inside OUR running guest (via Work:jobrun) and print its output.
#   job-w2.ps1 -Script "Stack 2000000`nWork:w2" [-Config w2-ref020.uae] [-TimeoutSec 300] [-NoWait]
# The script is written as next.tmp and renamed to next (jobrun only ever sees a complete file).
# LF line endings only - AmigaDOS does not run CRLF scripts. -NoWait queues it and prints the job number.
param([Parameter(Mandatory)][string]$Script, [string]$Config = "w2-ref020.uae", [int]$TimeoutSec = 300, [switch]$NoWait)
$cfg = Join-Path "C:\temp\amiga_w2" $Config
$workdir = ((Get-Content $cfg -ErrorAction SilentlyContinue | Where-Object { $_ -match '^filesystem2=.*WORK:Work:' } | Select-Object -First 1) -replace '^.*WORK:Work:', '' -replace ',-?[0-9]+$', '')
if (-not $workdir) { Write-Output "ERROR: $cfg missing (start-w2.ps1 -Config $Config first)"; exit 2 }
$jobs = Join-Path $workdir "jobs"
if (-not (Test-Path (Join-Path $jobs "jobrun.up"))) { Write-Output "ERROR: guest not up (start-w2.ps1 -Config $Config)"; exit 2 }
if (Test-Path (Join-Path $jobs "next")) { Write-Output "ERROR: a job is still queued"; exit 2 }
if (Test-Path (Join-Path $jobs "running")) { Write-Output "ERROR: a job is still running"; exit 2 }
$lastf = Join-Path $jobs "last"
$before = if (Test-Path $lastf) { [int](Get-Content $lastf) } else { 0 }
$tmp = Join-Path $jobs "next.tmp"
[IO.File]::WriteAllText($tmp, ($Script -replace "`r", "") + "`n")
Move-Item $tmp (Join-Path $jobs "next") -Force
$n = $before + 1
if ($NoWait) { Write-Output "queued job$n"; exit 0 }
$done = Join-Path $jobs "job$n.done"
$deadline = (Get-Date).AddSeconds($TimeoutSec)
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Milliseconds 500
  if (Test-Path $done) {
    Write-Output ("--- job{0} {1}" -f $n, (Get-Content $done))
    $log = Join-Path $jobs "job$n.log"
    if (Test-Path $log) { Get-Content $log }
    exit 0
  }
}
Write-Output "TIMEOUT: job$n not done after $TimeoutSec s (heartbeat: $(Get-Content (Join-Path $jobs 'heartbeat') -ErrorAction SilentlyContinue))"
exit 1
