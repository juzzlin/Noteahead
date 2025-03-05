# Noteahead

A simple MIDI tracker and sequencer for Linux focusing on ease of use. There are no audio tracks, only MIDI.

Written in Qt/QML/C++20 on top of RtMidi back-end. Builds with CMake and uses CTest + Qt Test framework for unit tests.

Noteahead is designed to be especially a MIDI tracker so it has/will have features that make MIDI sequencing as easy as possible, e.g. setting filter cutoff or changing patch on-the-fly without entering cryptic hex values on a panning column.

Noteahead is still a work in progress and there's **a lot of limitations and missing features**. However, I'm already using it for my own music.

<table>
  <tr>
    <td colspan="3"><img src="/screenshots/0.2.0/Noteahead_1.png" width="100%"></td>
  </tr>
  <tr>
    <td><img src="/screenshots/0.2.0/Noteahead_2.png" width="100%"></td>
    <td><img src="/screenshots/0.2.0/Noteahead_3.png" width="100%"></td>
  </tr>
 </table>

My first track composed with Noteahead:

<a href="https://www.youtube.com/watch?feature=player_embedded&v=Y_hq6uGTx9M">J.L.P - Right To Castle (YouTube)</a>

<a href="https://soundcloud.com/jussilindplays/right-to-castle">J.L.P - Right To Castle  (SoundCloud)</a>

##
## Features

* Accurate internal timing
* Fully scalable UI
* Cool volume meters like in NoiseTracker
* Cool note visualizer animation on the bottom bar
* Easy-to-use track editing
* Poor Man's MIDI Hot-Plug with automatic setup
  - Noteahead notices when a device goes online/offline
  - Noteahead automatically sets channel, bank, and patch
* Send MIDI clock on desired ports
* Tracks with multiple note columns
* Track settings with port, channel, bank, patch, volume, pan, cutoff
  - Can be easily changed on-the-fly via line events
* Track settings with generic MIDI CC slots
  - Initial MIDI CC values can be set directly via Track settings
* Track and column-specific velocity scales
  - Effective velocity is the product of track scale, column scale, and note velocity
* Saves to a custom (but open!) XML-based **.nahd** format

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

Only one "instrument" can be set per track (it's possible to change patch on-the-fly, however). I don't like the traditional concept where each note includes the instrument number and can be inserted anywhere. In my opinion this easily leads into a mess and the workflow is clumsy.

All values are entered in decimal format, typically ranging from 0 to 127 to align with the MIDI protocol and also used in many synthesizers. I've never been a fan of the hexadecimal notation commonly used in trackers.

### Internal timing strategy

The song is rendered into events just before playing. Accurate timestamps are calculated for each event beforehand in order to achieve a drifting-free timing. The player thread syncs to these event timestamps.

The timing accuracy is incredible when doing a multitrack recording (screenshot from Audacity):

<img src="/screenshots/Timing.png" width="100%">

### Project file format

Noteahead reads and saves to a custom XML-based format. This has several pros:

* I don't have to write a MIDI-file parser (yet)
* A plain text project file works well with version control systems like Git. You can immediately see what you have changed in the project. A binary-formatted project file is a black box and a PITA. 

Several..? That was only two, come on!

##
## License

Noteahead's source code is licensed under **GNU GPLv3**. See COPYING for the complete license text.

##
## Install instructions

Currently Debian packages for Ubuntu 24.04 and Ubuntu 24.10 are provided. Installation, for example:

    $ sudo apt install ./noteahead-0.1.0-ubuntu-24.04_amd64.deb

##
## Build instructions

Currently Noteahead depends on Qt >= 6.4. Actually that's the only thing that defines the Ubuntu version if stock Qt is to be used.

Noteahead is currently being developing on Ubuntu 22.04 LTS with the official Qt SDK and on Ubuntu 24.10 with the stock Qt from repositories.

###
### Build dependencies on Ubuntu 24.04+

Packages needed for building:

    build-essential cmake pkg-config ninja-build qt6-base-dev qt6-declarative-dev qt6-tools-dev librtmidi-dev

Additional packages needed to run:

    qml6-module-qtqml qml6-module-qtcore qml6-module-qtquick-dialogs qml6-module-qtquick-templates

###
### Build and run on CLI

    $ mkdir build && cd build

    $ cmake -GNinja ..

    $ ninja

    $ ./noteahead

Optionally install locally:

    $ ninja install

###
### Run unit tests on CLI

    $ ctest

###
### Create a Debian package

    $ cpack -G DEB

##
## Basic usage

When starting a new project, just click on the settings icon on a desired track and setup the MIDI device. I have all my synthesizers connected via USB.

Click on the track name to change it. `[+]`/`[-]` in the track header adds or removes note columns.

Press **ESC** to enter to edit mode and use your PC keyboard to input notes on a note column. The keyboard acts as a virtual "piano" like they usually do in tracker applications, **Z** is "C" on the lower octave.

When sequenced enough, press **SPACE** or use the play buttons to start playing.

Create a new pattern by increasing the value on the **PAT** spinner. Use **LEN** to set the pattern length.

Click on the pattern name to change it.

Use the Song section to set the play order of your patterns as well as the song length.

### Context menu

The main context menu can be accessed by right-clicking on the editor view.

Here you can cut/copy/paste, transpose, and set events on individual lines e.g. to change patch. The lines that have an event assigned will be rendered in a accent color.

### Most important "special" keys

* **ESC**: toggles the edit mode
* **SPACE**: toggles the play mode
* **INSERT**: inserts an empty line and moves subsequent lines down
* **BACKSPACE**: deletes the current line and pulls subsequent lines up
* **A**: inserts a note off event
* **F3**: decreases the current octave
* **F4**: increases the current octave
* **Z**..**M**: play/insert notes of the lower octave
* **Q**..**U**: play/insert notes of the higher octave

Cut/Copy/Paste (also available via right-clicking on the editor):

* **Alt + F3**: cut the current column
* **Alt + F4**: copy the current column
* **Alt + F5**: paste the copied column
* **Shift + F3**: cut the current track
* **Shift + F4**: copy the current track
* **Shift + F5**: paste the copied track
* **Ctrl + F3**: cut the current pattern
* **Ctrl + F4**: copy the current pattern
* **Ctrl + F5**: paste the copied pattern

Transposition (also available via right-clicking on the editor):

* **Alt + F9**: transpose column by +1 semitone
* **Alt + F10**: transpose column by -1 semitone
* **Alt + F11**: transpose column by +12 semitones
* **Alt + F12**: transpose column by -12 semitones
* **Shift + F9**: transpose track by +1 semitone
* **Shift + F10**: transpose track by -1 semitone
* **Shift + F11**: transpose track by +12 semitones
* **Shift + F12**: transpose track by -12 semitones
* **Ctrl + F9**: transpose pattern by +1 semitone
* **Ctrl + F10**: transpose pattern by -1 semitone
* **Ctrl + F11**: transpose pattern by +12 semitones
* **Ctrl + F12**: transpose pattern by -12 semitones

##
## Why am I doing this?

First of all, I'm a tracker musician and also a professional software engineer.

1) I don't want to depend on closed source software with closed file formats for my music work.

2) Because I can.

3) Because I want to live in the past.
