# BuildPackage.ps1 - Build ZBinary2CArray-WinUI3 as portable ZIP packages
# Usage:
#   .\BuildPackage.ps1              # Build all architectures (x86, x64, ARM64)
#   .\BuildPackage.ps1 -Arch x64   # Build single architecture
#   .\BuildPackage.ps1 -SkipRestore # Skip NuGet restore

param(
    [ValidateSet('x86', 'x64', 'ARM64', 'all')]
    [string]$Arch = 'all',
    [switch]$SkipRestore
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$projectFile = Join-Path $projectRoot 'ZBinary2CArray-WinUI3.vcxproj'
$configuration = 'Release'
$outputDir = Join-Path $projectRoot 'Package'

# --- Locate MSBuild via vswhere ---
$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    throw "vswhere not found at $vswhere"
}
$msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $msbuild) {
    throw 'MSBuild not found. Install Visual Studio with the Desktop C++ workload.'
}
Write-Host "MSBuild: $msbuild"

# --- NuGet restore ---
if (-not $SkipRestore) {
    $nuget = Get-Command nuget -ErrorAction SilentlyContinue
    if (-not $nuget) {
        throw 'nuget not found on PATH. Install NuGet CLI or dotnet.'
    }
    Write-Host 'Restoring NuGet packages...'
    & nuget restore $projectFile -NonInteractive -PackagesDirectory (Join-Path $projectRoot 'packages')
    if ($LASTEXITCODE -ne 0) { throw 'NuGet restore failed' }
}

# --- Determine architectures to build ---
if ($Arch -eq 'all') {
    $archs = @('x86', 'x64', 'ARM64')
} else {
    $archs = @($Arch)
}

# --- Create output directory ---
if (Test-Path $outputDir) {
    Get-ChildItem $outputDir -Filter '*.zip' | Remove-Item -Force
}
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

foreach ($arch in $archs) {
    $platform = if ($arch -eq 'x86') { 'Win32' } else { $arch }
    Write-Host "`n=== Building $arch (platform=$platform) ===" -ForegroundColor Cyan

    # Build self-contained, unpackaged (portable)
    & $msbuild $projectFile `
        /t:Build `
        /p:Configuration=$configuration `
        /p:Platform=$platform `
        /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $arch"
    }

    # Self-contained output: varies by platform — search for the exe
    # x64:  x64\Release\ZBinary2CArray-WinUI3\ZBinary2CArray_WinUI3.exe
    # ARM64: ARM64\Release\ZBinary2CArray-WinUI3\ZBinary2CArray_WinUI3.exe
    # Win32: Release\ZBinary2CArray-WinUI3\ZBinary2CArray_WinUI3.exe  (no platform prefix!)
    $exePath = Get-ChildItem $projectRoot -Filter 'ZBinary2CArray_WinUI3.exe' -Recurse |
        Where-Object { $_.FullName -match [regex]::Escape($configuration) -and $_.Directory.Name -eq 'ZBinary2CArray-WinUI3' } |
        Select-Object -First 1 -ExpandProperty FullName
    if (-not $exePath) {
        throw "Could not find ZBinary2CArray_WinUI3.exe after build"
    }
    $buildBinDir = Split-Path $exePath
    Write-Host "Build output: $buildBinDir"

    # --- Staging directory (folder name inside ZIP = ZBinary2CArray-WinUI3) ---
    $stagingRoot = Join-Path $outputDir "staging_$arch"
    $stagingDir = Join-Path $stagingRoot 'ZBinary2CArray-WinUI3'
    if (Test-Path $stagingRoot) { Remove-Item $stagingRoot -Recurse -Force }
    New-Item -ItemType Directory -Path $stagingDir -Force | Out-Null

    # --- Copy all files from self-contained build output (top level only, skip AppX subfolder) ---
    Get-ChildItem $buildBinDir -File | ForEach-Object {
        Copy-Item $_.FullName -Destination $stagingDir
    }

    # Copy subdirectories (Locales, Assets, etc.) but NOT AppX (it's a duplicate)
    Get-ChildItem $buildBinDir -Directory | Where-Object { $_.Name -ne 'AppX' } | ForEach-Object {
        Copy-Item $_.FullName -Destination $stagingDir -Recurse
    }

    # --- Ensure Locales are present (may be in build output or project root) ---
    $localesDest = Join-Path $stagingDir 'Locales'
    if (-not (Test-Path $localesDest)) {
        $localesSrc = Join-Path $projectRoot 'Locales'
        if (Test-Path $localesSrc) {
            Copy-Item $localesSrc -Destination $stagingDir -Recurse
        }
    }

    Write-Host "Staged contents:"
    Get-ChildItem $stagingDir -Recurse -File | ForEach-Object {
        $rel = $_.FullName.Substring($stagingDir.Length + 1)
        Write-Host "  $rel"
    }

    # --- Create ZIP ---
    $zipPath = Join-Path $outputDir "ZBinary2CArray-WinUI3_$arch.zip"
    Push-Location $stagingRoot
    try {
        Compress-Archive -Path 'ZBinary2CArray-WinUI3' -DestinationPath $zipPath -Force
    } finally {
        Pop-Location
    }
    Write-Host "Created: $zipPath" -ForegroundColor Green

    # --- Cleanup staging ---
    Remove-Item $stagingRoot -Recurse -Force
}

Write-Host "`n=== Done! ===" -ForegroundColor Green
Get-ChildItem $outputDir -Filter '*.zip' | ForEach-Object {
    $sizeMB = [math]::Round($_.Length / 1MB, 2)
    Write-Host "  $($_.Name) - $sizeMB MB"
}
