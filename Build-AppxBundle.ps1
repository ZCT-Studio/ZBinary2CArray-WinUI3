<#
.SYNOPSIS
    One-click build script for ZBinary2CArray-WinUI3.
    Builds Release APPX packages for x86, x64, and ARM64,
    then collects them into an AppxBundle/ folder.

.DESCRIPTION
    This script locates MSBuild, restores NuGet packages, builds the
    project in Release mode for each architecture, generates APPX
    packages, and copies the output to AppxBundle/.

.PARAMETER SkipRestore
    Skip NuGet package restore (use if packages are already restored).

.PARAMETER Arch
    Build only the specified architecture (x86, x64, or ARM64).
    If omitted, all three are built.

.EXAMPLE
    .\Build-AppxBundle.ps1
    Build all architectures.

.EXAMPLE
    .\Build-AppxBundle.ps1 -Arch x64
    Build only x64.
#>

[CmdletBinding()]
param(
    [switch]$SkipRestore,
    [ValidateSet('x86', 'x64', 'ARM64', 'all')]
    [string]$Arch = 'all'
)

$ErrorActionPreference = 'Stop'
$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectFile = Join-Path $ProjectRoot 'ZBinary2CArray-WinUI3.vcxproj'
$OutputDir   = Join-Path $ProjectRoot 'AppxBundle'

# --- Locate MSBuild ---
function Find-MSBuild {
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path $vswhere)) {
        $vswhere = "${env:ProgramFiles}\Microsoft Visual Studio\Installer\vswhere.exe"
    }
    if (-not (Test-Path $vswhere)) {
        throw "vswhere.exe not found. Please install Visual Studio 2022."
    }
    $msbuild = & $vswhere -latest -requires Microsoft.Component.MSBuild `
        -find "MSBuild\**\Bin\MSBuild.exe" | Select-Object -First 1
    if (-not $msbuild) {
        throw "MSBuild.exe not found. Please install the 'MSBuild' component in Visual Studio."
    }
    return $msbuild
}

# --- Map arch to MSBuild platform ---
function Get-PlatformName($a) {
    switch ($a) {
        'x86'   { return 'Win32' }
        'x64'   { return 'x64' }
        'ARM64' { return 'ARM64' }
        default { throw "Unknown arch: $a" }
    }
}

# --- Build one architecture ---
function Build-Arch($arch) {
    $platform = Get-PlatformName $arch
    Write-Host ""
    Write-Host "==========================================" -ForegroundColor Cyan
    Write-Host "  Building $arch ($platform) Release..."   -ForegroundColor Cyan
    Write-Host "==========================================" -ForegroundColor Cyan

    $args = @(
        $ProjectFile,
        '/t:Build',
        '/p:Configuration=Release',
        "/p:Platform=$platform",
        '/p:AppxPackageSigningEnabled=false',
        '/p:GenerateAppxPackageOnBuild=true',
        '/p:AppxBundlePlatforms=',
        '/v:minimal',
        '/nologo'
    )

    & $script:MSBuild $args
    if ($LASTEXITCODE -ne 0) {
        throw "Build failed for $arch (exit code $LASTEXITCODE)."
    }

    # Find the generated .appx file
    $appxPattern = Join-Path $ProjectRoot "$platform\Release\**\*.appx"
    # Win32 uses 'Win32' not 'x86' in the output path
    if ($arch -eq 'x86') {
        $appxPattern = Join-Path $ProjectRoot "Win32\Release\**\*.appx"
    } elseif ($arch -eq 'x64') {
        $appxPattern = Join-Path $ProjectRoot "x64\Release\**\*.appx"
    } elseif ($arch -eq 'ARM64') {
        $appxPattern = Join-Path $ProjectRoot "ARM64\Release\**\*.appx"
    }

    $appxFiles = Get-ChildItem -Path $appxPattern -Recurse -ErrorAction SilentlyContinue
    if (-not $appxFiles) {
        Write-Warning "No .appx file found for $arch. Checking AppxPackage dir..."
        # Fallback: search the whole Release output tree
        $searchBase = Join-Path $ProjectRoot ($platform + "\Release")
        $appxFiles = Get-ChildItem -Path $searchBase -Filter '*.appx' -Recurse -ErrorAction SilentlyContinue
    }

    return $appxFiles
}

# ============================================================
# Main
# ============================================================

Write-Host "ZBinary2CArray-WinUI3 — APPX Bundle Build Script" -ForegroundColor Green
Write-Host "Project root: $ProjectRoot" -ForegroundColor DarkGray

# Locate MSBuild
$script:MSBuild = Find-MSBuild
Write-Host "MSBuild: $script:MSBuild" -ForegroundColor DarkGray

# NuGet restore
if (-not $SkipRestore) {
    Write-Host ""
    Write-Host "Restoring NuGet packages..." -ForegroundColor Yellow
    nuget restore $ProjectFile -NonInteractive 2>$null
    if ($LASTEXITCODE -ne 0) {
        # Try msbuild restore as fallback
        & $script:MSBuild $ProjectFile /t:Restore /v:minimal /nologo
    }
}

# Determine architectures
if ($Arch -eq 'all') {
    $archs = @('x86', 'x64', 'ARM64')
} else {
    $archs = @($Arch)
}

# Create output directory
if (Test-Path $OutputDir) {
    Remove-Item $OutputDir -Recurse -Force
}
New-Item -ItemType Directory -Path $OutputDir -Force | Out-Null

# Build each architecture
$allAppx = @()
$failed = @()
foreach ($a in $archs) {
    try {
        $appxFiles = Build-Arch $a
        if ($appxFiles) {
            foreach ($f in $appxFiles) {
                $dest = Join-Path $OutputDir "ZBinary2CArray-WinUI3_$a.appx"
                Copy-Item $f.FullName $dest -Force
                $allAppx += $dest
                Write-Host "  -> $dest" -ForegroundColor Green
            }
        } else {
            Write-Warning "No .appx found for $a"
            $failed += $a
        }
    } catch {
        Write-Error "Failed to build ${a}: $_"
        $failed += $a
    }
}

# Summary
Write-Host ""
Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Build Summary" -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan
if ($allAppx.Count -gt 0) {
    Write-Host "Successfully built $($allAppx.Count) package(s):" -ForegroundColor Green
    foreach ($p in $allAppx) {
        Write-Host "  $p" -ForegroundColor Green
    }
}
if ($failed.Count -gt 0) {
    Write-Host "Failed: $($failed -join ', ')" -ForegroundColor Red
    exit 1
}
if ($allAppx.Count -eq 0) {
    Write-Host "No packages were built." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "Output directory: $OutputDir" -ForegroundColor Green
