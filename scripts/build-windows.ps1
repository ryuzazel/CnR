# Build CnR on Windows with CMake + MSVC, then copy it somewhere on PATH.
# Run from a "Developer PowerShell for VS" so cl.exe is available, or with MSVC installed.
# Usage:
#   .\scripts\build-windows.ps1
#   .\scripts\build-windows.ps1 -Portable   # skip /arch:AVX2, safer for distribution

param(
    [switch]$Portable
)

$ErrorActionPreference = "Stop"

$BuildDir = "build-windows"
Remove-Item -Recurse -Force $BuildDir -ErrorAction SilentlyContinue

# Note: MSVC has no direct -march=native equivalent. /arch:AVX2 is the closest
# broadly-safe "use modern SIMD" flag; CMakeLists.txt's -march=native check
# already no-ops on MSVC (it's a GCC/Clang flag), so nothing extra is needed
# there. Add /arch:AVX2 here if you want it and know target CPUs support it.
$ExtraArgs = @()
if (-not $Portable) {
    $ExtraArgs += "-DCMAKE_CXX_FLAGS=/arch:AVX2"
}

cmake -S . -B $BuildDir -DCMAKE_BUILD_TYPE=Release @ExtraArgs
cmake --build $BuildDir --config Release --parallel

$Exe = Join-Path $BuildDir "Release\CnR.exe"
if (-not (Test-Path $Exe)) { $Exe = Join-Path $BuildDir "CnR.exe" }

$InstallDir = "$Env:LOCALAPPDATA\Programs\CnR"
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null
Copy-Item $Exe -Destination "$InstallDir\CnR.exe" -Force

# Add to user PATH permanently if not already present
$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "User")
if ($CurrentPath -notlike "*$InstallDir*") {
    [Environment]::SetEnvironmentVariable("Path", "$CurrentPath;$InstallDir", "User")
    Write-Host "Added $InstallDir to your user PATH. Restart your terminal."
} else {
    Write-Host "$InstallDir already on PATH."
}

Write-Host "Done. Open a new terminal and try:  CnR `"args`""
