# Run the same controller code used by the ESPHome firmware, without hardware.
# The optional local compiler is in .esphome/test-tools and is not distributed.
# Install if needed: python -m pip install --target .esphome/test-tools ziglang==0.15.2
$ErrorActionPreference = 'Stop'
$waterRoot = Split-Path -Parent $PSScriptRoot
$waterCompiler = Join-Path $waterRoot '.esphome/test-tools/ziglang/zig.exe'
if (-not (Test-Path -LiteralPath $waterCompiler)) {
    throw 'Local test compiler missing. See the install command at the top of tests/run.ps1, or compile control_test.cpp with a C++17 compiler.'
}
$env:ZIG_LOCAL_CACHE_DIR = Join-Path $waterRoot '.esphome/zig-cache'
$env:ZIG_GLOBAL_CACHE_DIR = Join-Path $waterRoot '.esphome/zig-global-cache'
$waterExecutable = Join-Path $waterRoot '.esphome/control_test.exe'
& $waterCompiler c++ -std=c++17 -Wall -Wextra -Werror (Join-Path $PSScriptRoot 'control_test.cpp') -o $waterExecutable
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $waterExecutable
exit $LASTEXITCODE
