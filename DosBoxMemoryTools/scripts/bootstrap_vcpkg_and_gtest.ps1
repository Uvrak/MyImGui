# Bootstrap vcpkg and install gtest for solution
# Run this in PowerShell from the repo root.
$ErrorActionPreference = 'Stop'

if (-not (Test-Path "vcpkg")) {
	git clone https://github.com/microsoft/vcpkg.git vcpkg
}

Push-Location vcpkg
# run the bootstrap script and vcpkg.exe using the call operator
& .\bootstrap-vcpkg.bat
& .\vcpkg.exe install gtest:x64-windows
& .\vcpkg.exe integrate install
Pop-Location

Write-Host "Installed gtest via vcpkg (x64-windows). Update the DosBoxMemoryTools test project to link against it." 
