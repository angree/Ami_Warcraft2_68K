# release-win.ps1 - the two release archives OUTSIDE the repo, on Windows WITHOUT WSL (counterpart of release.sh):
#   powershell -ExecutionPolicy Bypass -File build\release-win.ps1 [-Out <dir>] [-ConverterFrom <old converter zip>]
#     <out>\Warcraft2_68K-<ver>.lha            Amiga drawer: w2, w2setup, icons, README, LICENSE
#     <out>\Warcraft2_68K-<ver>-converter.zip  PC data extractor (taken from an earlier release when the converter
#                                               sources did not change: it needs the Linux/WSL build otherwise)
# NO game data: build/release-gate.py refuses the trees if anything original or derived is inside.
param([string]$Out = "", [string]$ConverterFrom = "")
$ErrorActionPreference = "Stop"
$repo = Split-Path -Parent $PSScriptRoot
$ver = ((Select-String -Path (Join-Path $repo "src\version.h") -Pattern '^#define W2_VERSION "(.*)"').Matches[0].Groups[1].Value)
if (-not $ver) { throw "no version in src/version.h" }
if (-not $Out) { $Out = "D:\GITHUB\_AmiW2-poza-repo\dist\release-$ver" }
$stage = "D:\GITHUB\_AmiW2-poza-repo\wrelease"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Confirm:$false }
if (Test-Path $Out) { Remove-Item $Out -Recurse -Confirm:$false }
$D = Join-Path $stage "Warcraft2_68K"; $C = Join-Path $stage "conv\Warcraft2_68K-converter"
New-Item -ItemType Directory -Force (Join-Path $stage "build"), $D, $C, $Out | Out-Null
$lha = "C:\amiga-gcc\bin\lha.exe"

# 1. Amiga binaries + icons (never stripped)
& powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "build-win.ps1") -Deploy (Join-Path $stage "build") -DeployAros none -DeployPlay none > (Join-Path $stage "build.log") 2>&1
if (-not (Select-String -Path (Join-Path $stage "build.log") -Pattern "built and deployed" -Quiet)) { Get-Content (Join-Path $stage "build.log") -Tail 20; throw "BUILD FAILED" }
foreach ($f in "w2", "w2.info", "w2setup", "w2setup.info") { Copy-Item (Join-Path $stage "build\$f") (Join-Path $D $f) }
$bytes = [IO.File]::ReadAllBytes((Join-Path $D "w2")); $txt = [Text.Encoding]::ASCII.GetString($bytes)
if (-not $txt.Contains("Warcraft II 68K")) { throw "w2 has no name string" }
if (-not $txt.Contains($ver)) { throw "w2 does not carry version $ver" }
[IO.File]::WriteAllText((Join-Path $D "README.md"), ([IO.File]::ReadAllText((Join-Path $repo "README.md")) -replace "`r`n", "`n"))
Copy-Item (Join-Path $repo "LICENSE") (Join-Path $D "LICENSE")
& py -I -W ignore (Join-Path $repo "tools\mkicon_newicons.py") "D:\GITHUB\_AmiW2-poza-repo\wbuild\icons\w2.png" (Join-Path $stage "Warcraft2_68K.info") --drawer | Out-Null
if (-not (Test-Path (Join-Path $stage "Warcraft2_68K.info"))) { throw "drawer icon failed" }

# 2. the PC converter (unchanged sources: reuse the files of the given earlier release)
if (-not $ConverterFrom) { throw "-ConverterFrom <earlier ...-converter.zip> needed (the converter build needs WSL)" }
$tmpz = Join-Path $stage "oldconv"; Expand-Archive $ConverterFrom $tmpz
Copy-Item (Join-Path $tmpz "Warcraft2_68K-converter\*") $C -Recurse

# 3. gate: nothing original, nothing derived
& py -I (Join-Path $repo "build\release-gate.py") $D (Join-Path $stage "conv") (Join-Path $stage "Warcraft2_68K.info")
if ($LASTEXITCODE -ne 0) { throw "release gate refused" }

# 4. archives + control unpack
$L = Join-Path $Out "Warcraft2_68K-$ver.lha"; $Z = Join-Path $Out "Warcraft2_68K-$ver-converter.zip"
Push-Location $stage; & $lha ao5 $L Warcraft2_68K Warcraft2_68K.info | Out-Null; Pop-Location
if ($LASTEXITCODE -ne 0) { throw "lha failed" }
Compress-Archive -Path $C -DestinationPath $Z
$chk = Join-Path $stage "chk"; New-Item -ItemType Directory -Force $chk | Out-Null
Push-Location $chk; & $lha xq $L | Out-Null; Pop-Location
foreach ($f in (Get-ChildItem $D -File)) {
	$a = (Get-FileHash $f.FullName -Algorithm MD5).Hash; $b = (Get-FileHash (Join-Path $chk "Warcraft2_68K\$($f.Name)") -Algorithm MD5).Hash
	if ($a -ne $b) { throw "LHA CONTENT DIFFERS: $($f.Name)" }
}
Get-ChildItem $Out | Select-Object Name, Length | Format-Table -AutoSize | Out-String
Write-Output "release $ver ready in $Out"
