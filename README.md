# Noteahead

A simple MIDI tracker and sequencer for Linux focusing on ease of use. There are no audio tracks, only MIDI.

Written in Qt/QML/C++20 on top of RtMidi back-end. Builds with CMake and uses CTest + Qt Test framework for unit tests.

Noteahead is designed to be especially a MIDI tracker so it has/will have features that make MIDI sequencing as easy as possible, e.g. setting filter cutoff or changing patch on-the-fly without entering crypting hex values on a panning column.

Noteahead is still a work in progress. **DO NOT USE FOR ANY REAL WORK** (except for me, of course :).

<table>
  <tr>
    <td><img src="/screenshots/Noteahead_30-01-2025.png" width="100%"></td>
  </tr>
 </table>

##
## Features

* Very lightweight and fast
* Very accurate internal timing
* Fully scalable UI
* Cool volume meters like in NoiseTracker
* Easy-to-use track editing
* Poor Man's MIDI Hot-Plug with automatic setup
  - Noteahead notices when a device goes online/offline
  - Noteahead automatically sets channel, bank, and patch
* Unlimited number of tracks
* Unlimited number of note columns per track
* Saves to a custom (but open!) XML-based **.naxml** format

##
## Future dreams (**NOT YET IMPLEMENTED**):

* Horizontal visualization
* MIDI side-chaining (not sure if this is gonna work)
* MIDI CC automation (channel volume, pan, cut-off)
* Clock sync options: Internal, MIDI, Jack
* Video generation

##
## Some important design choices

### Software framework

Noteahead is written in Qt/QML/C++20, because it's a relevant, stable, and well-known technology stack especially in the Linux-world. Qt also has a very good support for safe threads and it performs well. The MIDI backend is currently based on RtMidi, but the architecture is such that it can be easily changed.

### Tracks and note columns

Only one "instrument" can be set per track (it will be possible to change patch on-the-fly, however). I don't like the traditional concept where each note includes the instrument number and can be inserted anywhere. In my opinion this easily leads into a mess and the workflow is clumsy.

### Internal timing strategy

The song is rendered into events just before playing. Accurate timestamps are calculated for each event beforehand in order to achieve a drifting-free timing. The player thread syncs to these event timestamps. Wow, that's A-M-A-Z-I-N-G! \o/

### Project file format

Noteahead reads and saves to a custom XML-based format. This has several pros:

* I don't have to write a MIDI-file parser (yet)
* A plain text project file works well with version control systems like Git. You can immediately see what you have changed in the project. A binary-formatted project file is a black box and a PITA. 

Several..? That was only two, come on!

##
## License

Noteahead's source code is licensed under **GNU GPLv3**. See COPYING for the complete license text.

##
## Build instructions

###
### Build dependencies on Ubuntu 24.04+

    build-essential cmake pkg-config ninja-build qt6-base-dev qt6-declarative-dev qt6-tools-dev librtmidi-dev

    qml6-module-qtqml qml6-module-qtcore qml6-module-qtquick-dialogs qml6-module-qtquick-templates [Runtime]

###
### Build and run on CLI

    $ mkdir build && cd build

    $ cmake -GNinja ..

    $ ninja

    $ ./noteahead

###
### Run unit tests on CLI

    $ ctest

##
## Basic usage

When starting a new project, just click on the settings icon on a desired track and setup the MIDI device. I have all my synthesizers connected via USB.

Click on the track name to change it. `[+]`/`[-]` in the track header adds or removes note columns.

Press **ESC** to enter to edit mode and use your PC keyboard to input notes on a note column. The keyboard acts as a virtual "piano" like they usually do in tracker applications, **Z** is "C" on the lower octave.

When sequenced enough, press **SPACE** or use the play buttons to start playing.

Create a new pattern by increasing the value on the **PAT** spinner. Use **LEN** to set the pattern length.

Click on the pattern name to change it.

### Most important "special" keys

* **ESC**: toggles the edit mode

* **SPACE**: toggles the play mode

* **A**: inserts a note off event

* **F3**: decreases the current octave

* **F4**: increases the current octave

* **Z**..**M**: notes of the lower octave

* **Q**..**U**: notes of the higher octave

##
## Why am I doing this?

First of all, I'm a tracker musician and also a professional software engineer.

1) I don't want to depend on closed source software with closed file formats for my music work.

2) Because I can.

3) Because I want to live in the past.
