param(
    [Parameter(Position=0)]
    [string]$BuildType = "web"
)

if ($BuildType -eq "web") {
    New-Item -ItemType Directory -Force -Path "build"
    Set-Location build
    emcmake cmake -DUSE_LOCAL=OFF -DCMAKE_TOOLCHAIN_FILE="$env:EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" ..
    emmake make
}
elseif ($BuildType -eq "native") {
    New-Item -ItemType Directory -Force -Path "build"
    Set-Location build
    cmake -G "MinGW Makefiles" ..
    make
}
elseif ($BuildType -eq "clean") {
    if (Test-Path "build") {
        Remove-Item -Path "build" -Recurse -Force
    }
}
else {
    Write-Error "Invalid argument. Use 'native', 'clean', 'web', 'local', or 'rebuild'."
    exit 1
}