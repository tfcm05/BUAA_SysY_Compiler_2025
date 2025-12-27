Param(
    [string]$Configuration = "Release",
    [string]$Generator = "MinGW Makefiles",
    [string]$CXX = "D:/c_byq/mingw64/bin/g++.exe",
    [switch]$Clean,
    [switch]$Run
)

$root = Split-Path -Path $PSScriptRoot -Parent
Push-Location $root

if ($Clean) {
    Write-Host "Removing build/ ..."
    Remove-Item -Recurse -Force .\build -ErrorAction SilentlyContinue
}

$cmakeArgs = @("-S", ".", "-B", "build", "-G", $Generator, "-DCMAKE_BUILD_TYPE=$Configuration")
if ($CXX) { $cmakeArgs += "-DCMAKE_CXX_COMPILER=$CXX" }

Write-Host "Configuring with: cmake $($cmakeArgs -join ' ')"
cmake @cmakeArgs
if ($LASTEXITCODE -ne 0) { Pop-Location; exit $LASTEXITCODE }

Write-Host "Building (config: $Configuration) ..."
cmake --build build --config $Configuration -- -j
if ($LASTEXITCODE -ne 0) { Pop-Location; exit $LASTEXITCODE }

if ($Run) {
    $exe1 = Join-Path $root "build\Compiler.exe"
    $exe2 = Join-Path $root "Compiler.exe"
    if (Test-Path $exe1) { & $exe1 }
    elseif (Test-Path $exe2) { & $exe2 }
    else { Write-Host "Executable not found: tried $exe1 and $exe2" }
}

Pop-Location
