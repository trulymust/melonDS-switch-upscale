#!/bin/bash

# Create a new build, I created it for the sake of simplicity
# You need bios and firmware to run it

cd build
make clean && cd .. && rm -rf build && mkdir build && cd build
cmake .. -DENABLE_OGLRENDERER=OFF -DBUILD_QT_SDL=OFF -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-cross-Switch.cmake -DCMAKE_BUILD_TYPE=Release
make -j$(nproc --all)
cp /home/gheovgos/VSCode/melonDS/build/melonDS.nro /home/gheovgos/melonds-switch/
