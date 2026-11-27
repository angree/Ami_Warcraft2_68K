# One headless run of w2 in OUR running guest: write w2.cfg + autoinput.txt, start w2 through the job queue,
# wait until the script has clicked, take a host screenshot of our window, wait for w2 to quit, print w2.log,
# convert the in-guest snapshot (PPM) to PNG. Nothing is typed or clicked on the host.
#   test-w2.ps1 -Tag ref_aga [-Config w2-ref020.uae] [-CfgLines "backend=aga","resolution=hires"] [-QuitMs 20000]
param([Parameter(Mandatory)][string]$Tag, [string]$Config = "w2-ref020.uae", [string[]]$CfgLines = @("backend=aga"),
      [int]$QuitMs = 20000, [int]$TimeoutSec = 400, [string[]]$Actions = @(), [int]$ShotMs = 4000)
$rt  = "D:\GITHUB\_AmiW2-poza-repo\emu"
$cfg = Join-Path $rt $Config
$workdir = ((Get-Content $cfg -ErrorAction SilentlyContinue | Where-Object { $_ -match '^filesystem2=.*WORK:Work:' } | Select-Object -First 1) -replace '^.*WORK:Work:', '' -replace ',-?[0-9]+$', '')
if (-not $workdir) { Write-Output "ERROR: start-w2.ps1 -Config $Config first"; exit 2 }
# powershell -File hands "a=1","b=2" over as ONE string "a=1,b=2": split it again
$CfgLines = @($CfgLines | ForEach-Object { $_ -split ',' } | Where-Object { $_ -ne "" })
$shots = Join-Path $rt "shots"
New-Item -ItemType Directory -Force $shots | Out-Null
$log = Join-Path $workdir "w2.log"
Remove-Item $log, (Join-Path $workdir "$Tag.ppm") -ErrorAction SilentlyContinue        # freopen("w") does not truncate
[IO.File]::WriteAllText((Join-Path $workdir "w2.cfg"), (($CfgLines + "show_fps=1") -join "`n") + "`n")
if ($Actions.Count -gt 0) {   # own actions ("<ms> <action> [args]", ';'-separated works too); the host shot comes at ShotMs
  $Actions = @($Actions | ForEach-Object { $_ -split ';' } | Where-Object { $_.Trim() -ne "" })
  $ain = @("# test-w2.ps1 $Tag") + $Actions + @("$ShotMs log ready for the host screenshot", "$($ShotMs + 5000) snap PROGDIR:$Tag.ppm", "$QuitMs quit")
} else {
$ain = @("# test-w2.ps1 $Tag", "1500 move 100 100", "2500 click 320 200", "3000 rclick 200 150", "3500 key 0x40",
         "4000 log ready for the host screenshot", "9000 snap PROGDIR:$Tag.ppm", "$QuitMs quit")
}
[IO.File]::WriteAllText((Join-Path $workdir "autoinput.txt"), ($ain -join "`n") + "`n")

$q = & "$PSScriptRoot\job-w2.ps1" -Config $Config -Script "Stack 2000000`nWork:w2`nEcho `"w2 rc `$RC`"" -NoWait
Write-Output $q
if ($q -notlike "queued job*") { exit 2 }
$n = [int]($q -replace '\D', '')
$deadline = (Get-Date).AddSeconds($TimeoutSec)
$shot = $false
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Seconds 2
  $t = if (Test-Path $log) { Get-Content $log -Raw -ErrorAction SilentlyContinue } else { "" }
  if (-not $shot -and $t -match "ready for the host screenshot") {
    Start-Sleep -Seconds 2
    & "$PSScriptRoot\shot-w2.ps1" -Config $Config -Out (Join-Path $shots "$Tag.png")
    $shot = $true
  }
  if (Test-Path (Join-Path $workdir "jobs\job$n.done")) { break }
}
Remove-Item (Join-Path $workdir "autoinput.txt") -ErrorAction SilentlyContinue   # a manual run must not quit by itself
Write-Output "--- job$n $(Get-Content (Join-Path $workdir "jobs\job$n.done") -ErrorAction SilentlyContinue)"
Get-Content (Join-Path $workdir "jobs\job$n.log") -ErrorAction SilentlyContinue
Write-Output "--- $log"
Get-Content $log -ErrorAction SilentlyContinue

# the guest's own snapshot (amigagfx_dump_screen, PPM P6) -> PNG next to the host shot
$ppm = Join-Path $workdir "$Tag.ppm"
if (Test-Path $ppm) {
  Add-Type -AssemblyName System.Drawing
  $b = [IO.File]::ReadAllBytes($ppm)
  $i = 0; $f = @()
  while ($f.Count -lt 4) { $s = ""; while ([char]$b[$i] -match '\s') { $i++ }; while (-not ([char]$b[$i] -match '\s')) { $s += [char]$b[$i]; $i++ }; $f += $s }
  $i++
  $w = [int]$f[1]; $h = [int]$f[2]
  $bmp = New-Object System.Drawing.Bitmap $w, $h, ([System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
  $bd = $bmp.LockBits((New-Object System.Drawing.Rectangle 0, 0, $w, $h), [System.Drawing.Imaging.ImageLockMode]::WriteOnly, $bmp.PixelFormat)
  $row = New-Object byte[] ($bd.Stride)
  for ($y = 0; $y -lt $h; $y++) {
    for ($x = 0; $x -lt $w; $x++) { $o = $i + ($y * $w + $x) * 3; $row[$x * 3] = $b[$o + 2]; $row[$x * 3 + 1] = $b[$o + 1]; $row[$x * 3 + 2] = $b[$o] }
    [Runtime.InteropServices.Marshal]::Copy($row, 0, [IntPtr]($bd.Scan0.ToInt64() + $y * $bd.Stride), $bd.Stride)
  }
  $bmp.UnlockBits($bd)
  $png = Join-Path $shots "$Tag-guest.png"
  $bmp.Save($png, [System.Drawing.Imaging.ImageFormat]::Png)
  Write-Output "guest snapshot: $png (${w}x${h})"
} else { Write-Output "no guest snapshot ($ppm)" }
