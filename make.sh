#!/bin/bash


if [ $# -eq 0 ] || [ "$1" = "web" ]; then
    mkdir -p build
    cd build 
    emcmake cmake -DUSE_LOCAL=OFF -DCMAKE_TOOLCHAIN_FILE="$EMSDK/upstream/emscripten/cmake/Modules/Platform/Emscripten.cmake" ..
    emmake make
elif [ "$1" = "native" ]; then
    mkdir -p build
    cd build
    cmake ..
    make
elif [ "$1" = "clean" ]; then
    rm -rf build
else
    echo "Invalid argument. Use 'native', 'clean', 'web', 'local', or 'rebuild'."
    exit 1
fi
