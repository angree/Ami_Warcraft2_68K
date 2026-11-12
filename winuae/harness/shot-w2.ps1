# Screenshot OUR guest window with PrintWindow (works when covered; takes control of nothing).
#   shot-w2.ps1 -Out C:\temp\amiga_w2\shots\x.png [-Config w2-ref020.uae]
# Prints the number of distinct colours sampled: exactly 1 means the capture failed, not the program.
param([Parameter(Mandatory)][string]$Out, [string]$Config = "w2-ref020.uae")
$OurExe = "C:\temp\amiga_w2\uae\winuae-w2.exe"   # the ONLY executable these scripts may touch: exact ExecutablePath
# AND C:\temp\amiga_w2 + our config in the command line. Never by name, window title or "winuae" match.
Add-Type -AssemblyName System.Drawing
Add-Type -Name P -Namespace W2S -MemberDefinition @'
[DllImport("user32.dll")] public static extern bool PrintWindow(IntPtr h, IntPtr dc, uint f);
[DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr h, out RECT r);
public struct RECT { public int L, T, R, B; }
public delegate bool EnumProc(IntPtr h, IntPtr l);
[DllImport("user32.dll")] public static extern bool EnumWindows(EnumProc p, IntPtr l);
[DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint wp);
[DllImport("user32.dll")] public static extern int GetClassName(IntPtr h, System.Text.StringBuilder s, int n);
'@
$cfg = Join-Path "C:\temp\amiga_w2" $Config
$procs = @(Get-CimInstance Win32_Process -Filter "Name = 'winuae-w2.exe'" | Where-Object { $_.ExecutablePath -eq $OurExe -and $_.CommandLine -like '*C:\temp\amiga_w2*' -and $_.CommandLine -like "*$cfg*" } | ForEach-Object { [uint32]$_.ProcessId })
if ($procs.Count -eq 0) { Write-Output "not running"; exit 1 }
$script:hw = [IntPtr]::Zero
$cb = [W2S.P+EnumProc]{ param($h, $l)
  [uint32]$wp = 0; [W2S.P]::GetWindowThreadProcessId($h, [ref]$wp) | Out-Null
  if ($procs -contains $wp) { $cn = New-Object System.Text.StringBuilder 64; [W2S.P]::GetClassName($h, $cn, 64) | Out-Null
    if ($cn.ToString() -eq 'PCsuxRox') { $script:hw = $h } }
  return $true }
[W2S.P]::EnumWindows($cb, [IntPtr]::Zero) | Out-Null
if ($script:hw -eq [IntPtr]::Zero) { Write-Output "no window"; exit 1 }
$r = New-Object W2S.P+RECT
[W2S.P]::GetClientRect($script:hw, [ref]$r) | Out-Null
$bmp = New-Object System.Drawing.Bitmap ([Math]::Max(1, $r.R)), ([Math]::Max(1, $r.B))
$g = [System.Drawing.Graphics]::FromImage($bmp)
$dc = $g.GetHdc(); [W2S.P]::PrintWindow($script:hw, $dc, 3) | Out-Null; $g.ReleaseHdc($dc)
$cols = @{}
for ($y = 0; $y -lt $bmp.Height; $y += 7) { for ($x = 0; $x -lt $bmp.Width; $x += 7) { $cols[$bmp.GetPixel($x, $y).ToArgb()] = 1 } }
New-Item -ItemType Directory -Force (Split-Path $Out) | Out-Null
$bmp.Save($Out, [System.Drawing.Imaging.ImageFormat]::Png)
Write-Output "saved $Out ($($r.R)x$($r.B), $($cols.Count) colours sampled)"
