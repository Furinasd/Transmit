# Run on Windows with UE 5.8 and its supported Visual Studio / Windows SDK installed.
[CmdletBinding()]
param(
    [string]$EngineDir = $env:TRANSMIT_ENGINE_DIR,
    [string]$OutputDir,
    [ValidateSet('Development', 'Shipping')]
    [string]$Configuration = 'Development'
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
if ($env:OS -ne 'Windows_NT') { throw 'Win64 packaging requires a Windows host.' }
$repo = Split-Path $PSScriptRoot -Parent
if (-not $EngineDir) { $EngineDir = 'C:\Program Files\Epic Games\UE_5.8' }
$uat = Join-Path $EngineDir 'Engine\Build\BatchFiles\RunUAT.bat'
if (-not (Test-Path $uat -PathType Leaf)) { throw "RunUAT not found: $uat" }
if (-not $OutputDir) {
    $OutputDir = Join-Path $repo ('Saved\LTransmitCandidate\' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-Win64')
}
$OutputDir = [IO.Path]::GetFullPath($OutputDir)
if (Test-Path $OutputDir) { throw "Refusing to overwrite candidate: $OutputDir" }
$head = & git -C $repo rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot determine source commit.' }
$status = & git -C $repo status --porcelain
if ($LASTEXITCODE -ne 0) { throw 'Cannot inspect source status.' }
if ($status) { throw 'Commit intended changes before packaging; source worktree must be clean.' }
& git -C $repo lfs version
if ($LASTEXITCODE -ne 0) { throw 'Install Git LFS before packaging.' }
& git -C $repo lfs pull
if ($LASTEXITCODE -ne 0) { throw 'Git LFS download failed.' }
New-Item -ItemType Directory -Path $OutputDir | Out-Null
$head | Set-Content (Join-Path $OutputDir 'source-head.txt')
'clean' | Set-Content (Join-Path $OutputDir 'source-status.txt')
$uatArgs = @(
    'BuildCookRun', "-project=$repo\passely.uproject", '-noP4', '-platform=Win64',
    "-clientconfig=$Configuration", '-build', '-cook', '-stage', '-pak', '-iostore',
    '-archive', "-archivedirectory=$OutputDir\Package", "-stagingdirectory=$OutputDir\Stage",
    '-map=/Game/Transmit/Maps/L_Transmit',
    # The cooker needs no MCP listener; an interactive Editor may already own its port.
    '-AdditionalCookerOptions=-SkipZenStore -ini:EditorPerProjectUserSettings:[/Script/ModelContextProtocolEngine.ModelContextProtocolSettings]:bAutoStartServer=False',
    '-prereqs', '-unattended', '-utf8output'
)
$uatArgs | Set-Content (Join-Path $OutputDir 'uat-arguments.txt')
& $uat @uatArgs 2>&1 | Tee-Object -FilePath (Join-Path $OutputDir 'package.log')
$uatExit = $LASTEXITCODE
if ($uatExit -ne 0) { throw "BuildCookRun failed ($uatExit); see package.log." }
$package = Join-Path $OutputDir 'Package\Windows'
$launcher = Join-Path $package 'passely.exe'
$gameName = if ($Configuration -eq 'Shipping') { 'passely-Win64-Shipping.exe' } else { 'passely.exe' }
$game = Join-Path $package "passely\Binaries\Win64\$gameName"
$paks = Join-Path $package 'passely\Content\Paks'
foreach ($path in @($launcher, $game)) {
    if (-not (Test-Path $path -PathType Leaf)) { throw "Packaged executable missing: $path" }
}
foreach ($extension in @('*.pak', '*.utoc', '*.ucas')) {
    if (-not (Get-ChildItem $paks -Filter $extension -File)) { throw "Missing cooked payload: $extension" }
}
$headAfter = & git -C $repo rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $headAfter -ne $head) { throw 'Source commit changed during packaging.' }
$statusAfter = & git -C $repo status --porcelain
if ($LASTEXITCODE -ne 0 -or $statusAfter) { throw 'Source worktree changed during packaging.' }
Get-ChildItem $package -Recurse -File | ForEach-Object {
    $hash = Get-FileHash $_.FullName -Algorithm SHA256
    [PSCustomObject]@{ Path = $_.FullName.Substring($package.Length + 1); SHA256 = $hash.Hash; Bytes = $_.Length }
} | Export-Csv (Join-Path $OutputDir 'package-sha256.csv') -NoTypeInformation
@"
Transmit Win64 $Configuration
Source: $head
Launch Package\Windows\passely.exe. Distribute the ENTIRE Windows folder, not the EXE alone.
WASD / mouse / Space; E capture; Q transfer; Tab help; Backspace local retry; R restart.
Build/cook/archive and payload checks passed. Gameplay and visual acceptance remain manual.
Known open issue: entering the arena carrying ordinary Motion can block Boss Motion capture.
See Docs/submission/KNOWN_GAPS.md and Docs/PACKAGING.md for acceptance gates.
"@ | Set-Content (Join-Path $OutputDir 'PLAYTEST.txt')
Write-Host "Candidate: $launcher"
