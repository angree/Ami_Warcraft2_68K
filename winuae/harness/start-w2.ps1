# Start OUR WinUAE guest for the AmiW2 port ONCE and leave it running; tests go in through job-w2.ps1.
#   powershell -ExecutionPolicy Bypass -File winuae\harness\start-w2.ps1 [-Config w2-ref020.uae|w2-aros.uae|w2-play.uae]
# Rules (!instrukcje 90-testowanie-emulatory.md): one instance per config, found by the FULL path of its
# config (C:\temp\amiga_w2\<cfg>) on the command line - never by process name; started DETACHED via WMI so
# an interrupted agent shell cannot take it down; the window is shown WITHOUT activation (a minimised
# WinUAE pauses / renders black); never synthesises host input; touches nothing outside C:\temp\amiga_w2.
param([string]$Config = "w2-ref020.uae", [int]$TimeoutSec = 240)
$OurExe = "C:\temp\amiga_w2\uae\winuae-w2.exe"   # the ONLY executable these scripts may touch: exact ExecutablePath
# AND C:\temp\amiga_w2 + our config in the command line. Never by name, window title or "winuae" match.
$rt   = "C:\temp\amiga_w2"
$exe  = Join-Path $rt "uae\winuae-w2.exe"
$src  = Join-Path (Split-Path -Parent $PSScriptRoot) $Config
$cfg  = Join-Path $rt $Config
if (-not (Test-Path $exe)) { Write-Output "ERROR: $exe missing"; exit 4 }
if (-not (Test-Path $src)) { Write-Output "ERROR: $src missing"; exit 4 }
$mine = @(Get-CimInstance Win32_Process -Filter "Name = 'winuae-w2.exe'" | Where-Object { $_.ExecutablePath -eq $OurExe -and $_.CommandLine -like '*C:\temp\amiga_w2*' -and $_.CommandLine -like "*$cfg*" })
if ($mine.Count -gt 0) { Write-Output ("ALREADY RUNNING: pid {0} - use job-w2.ps1" -f $mine[0].ProcessId); exit 3 }
Copy-Item $src $cfg -Force

# Work: comes from the config itself (each guest has its own folder and its own job queue)
$workdir = ((Get-Content $cfg | Where-Object { $_ -match '^filesystem2=.*WORK:Work:' } | Select-Object -First 1) -replace '^.*WORK:Work:', '' -replace ',-?[0-9]+$', '')
if (-not $workdir) { Write-Output "ERROR: no WORK:Work: mount in $cfg"; exit 5 }
if (-not $workdir.StartsWith($rt)) { Write-Output "ERROR: Work: $workdir is outside $rt - refusing"; exit 5 }
$jobs = Join-Path $workdir "jobs"
New-Item -ItemType Directory -Force $jobs | Out-Null
# jobrun numbers its jobs from 1 again after every boot: old results must go
Get-ChildItem $jobs -File -ErrorAction SilentlyContinue | Remove-Item -ErrorAction SilentlyContinue

$si = New-CimInstance -ClassName Win32_ProcessStartup -ClientOnly -Property @{ ShowWindow = [uint16]4 }
$r = Invoke-CimMethod -ClassName Win32_Process -MethodName Create -Arguments @{
  CommandLine = ('"{0}" -log -f "{1}"' -f $exe, $cfg); CurrentDirectory = (Split-Path $exe); ProcessStartupInformation = $si }
if ($r.ReturnValue -ne 0) { Write-Output "Win32_Process.Create failed: $($r.ReturnValue)"; exit 1 }
Write-Output "started pid $($r.ProcessId) $cfg"

Add-Type -Name W -Namespace W2H -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool ShowWindowAsync(IntPtr h, int c);
public delegate bool EnumProc(IntPtr h, IntPtr l);
[DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint wp);
[DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, System.Text.StringBuilder s, int n);
[DllImport("user32.dll")] public static extern bool ClipCursor(IntPtr r);
'@
$deadline = (Get-Date).AddSeconds($TimeoutSec)
$script:shown = $false
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Milliseconds 700
  [W2H.W]::ClipCursor([IntPtr]::Zero) | Out-Null          # never let the guest keep the host's mouse
  if (-not $script:shown) {
    $procs = @(Get-CimInstance Win32_Process -Filter "Name = 'winuae-w2.exe'" | Where-Object { $_.ExecutablePath -eq $OurExe -and $_.CommandLine -like '*C:\temp\amiga_w2*' -and $_.CommandLine -like "*$cfg*" } | ForEach-Object { [uint32]$_.ProcessId })
    $cb = [W2H.W+EnumProc]{ param($h, $l)
      [uint32]$wp = 0; [W2H.W]::GetWindowThreadProcessId($h, [ref]$wp) | Out-Null
      if ($procs -contains $wp) { $cn = New-Object System.Text.StringBuilder 64; [W2H.W]::GetClassName($h, $cn, 64) | Out-Null
        if ($cn.ToString() -eq 'PCsuxRox') { [W2H.W]::ShowWindowAsync($h, 4) | Out-Null; $script:shown = $true } }   # SW_SHOWNOACTIVATE
      return $true }
    [W2H.W]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
  }
  if (Test-Path (Join-Path $jobs "jobrun.up")) { Write-Output "guest up: jobrun is polling $jobs (window shown: $script:shown)"; exit 0 }
  if ($Config -like "*play*" -and $script:shown) { Write-Output "play guest started (no job queue)"; exit 0 }
}
Write-Output "TIMEOUT: no jobs\jobrun.up after $TimeoutSec s (guest left running - stop-w2.ps1 -Config $Config)"
exit 1
