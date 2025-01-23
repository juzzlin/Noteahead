# Cacophony

A simple pattern-based MIDI tracker for Linux. There are no audio tracks, only MIDI.

Written in Qt/QML/C++20 on top of RtMidi back-end. Builds with CMake and uses CTest + Qt Test framework for unit tests.

Cacophony is still a work in progress. **DO NOT USE FOR ANY REAL WORK** (except for me, of course :).

##
## Features

* Very lightweight and fast
* Very accurate internal timing
* Fully scalable UI
* Cool volume meters like in NoiseTracker
* Easy-to-use track editing
* Poor Man's MIDI Hot-Plug with automatic setup
  - Cacophony notices when a device goes online/offline
  - Cacophony automatically sets channel, bank, and patch
* Unlimited number of tracks
* Unlimited number of note columns per track
* Saves to a custom (but open!) XML-based **.caco** format

##
## Future dreams (**NOT YET IMPLEMENTED**):

* Horizontal visualization
* MIDI side-chaining (not sure if this is gonna work)
* MIDI CC automation (channel volume, pan, cut-off)
* Clock sync options: Internal, MIDI, Jack
* Video generation

##
## License

Cacophony's source code is licensed under GNU GPLv3. See COPYING for the complete license text.

##
## Build dependencies on Ubuntu 24.04+

    build-essential cmake pkg-config ninja-build qt6-base-dev qt6-declarative-dev qt6-tools-dev librtmidi-dev

    qml6-module-qtqml qml6-module-qtcore qml6-module-qtquick-dialogs qml6-module-qtquick-templates [Runtime]

##
## Build and run on CLI

    mkdir build && cd build

    cmake -GNinja ..

    ninja

    ./cacophony

##
## Run unit tests on CLI

    ctest

##
## Why am I doing this?

First of all, I'm a tracker musician and also a professional software engineer.

1) I don't want to depend on closed source software with closed file formats for my music work.

2) Because I can.

3) Because I want to live in the past.
