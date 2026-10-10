# Mathan Mart Server Launcher
# Adds required runtime DLL paths to PATH and sets PG_PASSWORD if not already set in the environment

$env:PATH = "C:\msys64\ucrt64\bin;C:\Program Files\PostgreSQL\18\bin;" + $env:PATH

if (-not $env:PG_PASSWORD) {
    $env:PG_PASSWORD = "1756"
}

Write-Host "Starting Mathan Mart Backend on http://localhost:8080..." -ForegroundColor Cyan
Set-Location -Path $PSScriptRoot
& "$PSScriptRoot\build\backend\MathanMartBackend.exe"
