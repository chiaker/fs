<#
PowerShell build script for the dating_server project.
Usage: run this from project root in PowerShell:
    .\build.ps1
This script expands the C++ source files in `src\` and calls g++.
#>

$ErrorActionPreference = 'Stop'

# find .cpp files
$files = Get-ChildItem -Path .\src -Filter *.cpp | ForEach-Object { $_.FullName }
if ($files.Count -eq 0) {
    Write-Error "No .cpp files found in src\"
    exit 1
}

Write-Host "Compiling files:`n" ($files -join "`n") -ForegroundColor Cyan

# If a previous server process is running, stop it so the linker can write the EXE
try {
    $proc = Get-Process -Name dating_server -ErrorAction SilentlyContinue
    if ($proc) {
        Write-Host "Found running 'dating_server.exe' process. Stopping it to allow build to proceed..." -ForegroundColor Yellow
        $proc | Stop-Process -Force
        Start-Sleep -Milliseconds 200
    }
} catch {
    Write-Host "Warning: failed to stop existing dating_server process: $_" -ForegroundColor Yellow
}

# remove existing binary if present (helps with file locks)
try { Remove-Item -Path .\dating_server.exe -ErrorAction SilentlyContinue } catch {}

& g++ -std=c++17 -O2 -I./src $files -o dating_server.exe -lws2_32

if ($LASTEXITCODE -eq 0) { Write-Host "Build succeeded: .\dating_server.exe" -ForegroundColor Green }
else { Write-Error "Build failed with exit code $LASTEXITCODE" }
