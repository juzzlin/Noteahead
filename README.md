# Cacophony

A simple pattern-based MIDI tracker for Linux. There are no audio tracks, only MIDI.

Written in Qt/QML/C++20 on top of RtMidi back-end. Builds with CMake and uses CTest + Qt Test framework for unit tests.

## Features (**NOT ALL YET IMPLEMENTED!**):

* Very lightweight and fast
* Fully scalable UI
* Velocity bars
* Horizontal visualization
* MIDI side-chaining (not sure if this is gonna work)
* Easy-to-use track editing 
* Unlimited number of tracks
* Unlimited number of note columns per track
* MIDI CC automation (channel volume, pan, cut-off)
* Clock sync options: Internal, MIDI, Jack
* Saves to a custom (but open!) .caco format

Cacophony is still a work in progress. **DO NOT USE FOR ANY REAL WORK** (except me, of course :).

## License

Cacophony's source code is licensed under GNU GPLv3. See COPYING for the complete license text.

## Build dependencies on Ubuntu 24.04+

    build-essential cmake pkg-config ninja-build qt6-base-dev qt6-declarative-dev qt6-tools-dev librtmidi-dev

    qml6-module-qtqml qml6-module-qtquick-templates [Runtime]

## Build and run on CLI

    mkdir build && cd build

    cmake -GNinja ..

    ninja

    ./cacophony

## Run tests on CLI

    ctest

## Why am I doing this?

First of all, I'm a tracker musician and also a professional software engineer.

1) I'm doing this because I'm crazy and I can. While I love Renoise I find its MIDI features a bit clumsy to use and something that could be improved.

2) I don't want to depend on closed source software with closed file formats for my music work.

