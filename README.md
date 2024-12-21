# Cacophony

A simple MIDI tracker for Linux. There are no audio tracks, only MIDI.

## Build dependencies on Ubuntu 24.04+

    build-essential cmake pkg-config ninja-build qt6-base-dev qt6-declarative-dev qt6-tools-dev librtmidi-dev

    qml6-module-qtqml qml6-module-qtquick-templates [Runtime]

## Build and run on CLI

    mkdir build

    cd build

    cmake -GNinja

    ninja

    ./cacophony

