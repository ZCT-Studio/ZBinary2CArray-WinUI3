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
$archs = if ($Arch -eq 'all') { @('x86', 'x64', 'ARM64') } else @($Arch)

# --- Create output directory ---
if (Test-Path $outputDir) {
    Get-ChildItem $outputDir -Filter '*.zip' | Remove-Item -Force
}
New-Item -ItemType Directory -Path $outputDir -Force | Out-Null

# --- VC++ Redist DLLs to bundle ---
$vcRuntimeDlls = @('vcruntime140.dll', 'msvcp140.dll', 'vcruntime140_1.dll', 'concrt140.dll')

foreach ($arch in $archs) {
    $platform = if ($arch -eq 'x86') { 'Win32' } else { $arch }
    Write-Host "`n=== Building $arch (platform=$platform) ===" -ForegroundColor Cyan

    # Build unpackaged (no APPX), self-contained, bootstrap auto-init
    & $msbuild $projectFile `
        /t:Build `
        /p:Configuration=$configuration `
        /p:Platform=$platform `
        /p:AppxPackage=false `
        /p:WindowsAppSdkBootstrapInitialize=true `
        /p:WindowsAppSDKSelfContained=true `
        /p:UseVCLibStl=false `
        /v:minimal /nologo
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $arch"
    }

    $buildOutput = Join-Path $projectRoot "$platform\$configuration"
    if (-not (Test-Path $buildOutput)) {
        throw "Build output not found: $buildOutput"
    }

    # --- Staging directory ---
    $stagingDir = Join-Path $outputDir "staging\$arch\ZBinary2CArray-WinUI3"
    if (Test-Path $stagingDir) { Remove-Item $stagingDir -Recurse -Force }
    New-Item -ItemType Directory -Path $stagingDir -Force | Out-Null

    # --- Copy EXE and DLLs from build output ---
    Write-Host "Copying build output from $buildOutput"
    Get-ChildItem $buildOutput -File | Where-Object {
        $_.Extension -in '.exe', '.dll', '.json', '.winmd'
    } | ForEach-Object {
        Copy-Item $_.FullName -Destination $stagingDir
    }

    # --- Copy Locales folder ---
    $localesSrc = Join-Path $buildOutput 'Locales'
    if (-not (Test-Path $localesSrc)) {
        $localesSrc = Join-Path $projectRoot 'Locales'
    }
    if (Test-Path $localesSrc) {
        Copy-Item $localesSrc -Destination $stagingDir -Recurse
    }

    # --- Copy Assets folder ---
    $assetsSrc = Join-Path $projectRoot 'Assets'
    if (Test-Path $assetsSrc) {
        Copy-Item $assetsSrc -Destination $stagingDir -Recurse
    }

    # --- Ensure VC++ runtime DLLs are present ---
    foreach ($dllName in $vcRuntimeDlls) {
        $destDll = Join-Path $stagingDir $dllName
        if (-not (Test-Path $destDll)) {
            # Try VC++ redist directory
            $redistBase = & $vswhere -latest -find 'VC\Redist\MSVC\*' | Sort-Object -Descending | Select-Object -First 1
            if ($redistBase) {
                $redistArch = if ($arch -eq 'ARM64') { 'arm64' } elseif ($arch -eq 'x86') { 'x86' } else { 'x64' }
                $redistCrt = Join-Path $redistBase "$redistArch\Microsoft.VC143.CRT"
                $srcDll = Join-Path $redistCrt $dllName
                if (Test-Path $srcDll) {
                    Copy-Item $srcDll -Destination $destDll
                    Write-Host "  Copied $dllName from VC++ redist"
                }
            }
        }
    }

    # --- List contents ---
    Write-Host "Staged contents:"
    Get-ChildItem $stagingDir -Recurse | ForEach-Object {
        $rel = $_.FullName.Substring($stagingDir.Length + 1)
        Write-Host "  $rel"
    }

    # --- Create ZIP (preserve ZBinary2CArray-WinUI3 folder name) ---
    $zipPath = Join-Path $outputDir "ZBinary2CArray-WinUI3_$arch.zip"
    $stagingParent = Split-Path -Parent $stagingDir
    Push-Location $stagingParent
    try {
        Compress-Archive -Path 'ZBinary2CArray-WinUI3' -DestinationPath $zipPath -Force
    } finally {
        Pop-Location
    }
    Write-Host "Created: $zipPath" -ForegroundColor Green
}

# --- Cleanup staging ---
$stagingRoot = Join-Path $outputDir 'staging'
if (Test-Path $stagingRoot) { Remove-Item $stagingRoot -Recurse -Force }

Write-Host "`n=== Done! ===" -ForegroundColor Green
Get-ChildItem $outputDir -Filter '*.zip' | ForEach-Object {
    $sizeMB = [math]::Round($_.Length / 1MB, 2)
    Write-Host "  $($_.Name) - $sizeMB MB"
}
