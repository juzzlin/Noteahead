# Noteahead

![Build Status](https://github.com/juzzlin/Noteahead/actions/workflows/ci.yml/badge.svg)

Noteahead is a "simple" MIDI tracker and sequencer for Linux focusing on ease of use. There are no audio tracks, only MIDI.

Noteahead is designed to be especially a MIDI tracker, so it has/will have features that make MIDI sequencing as easy as possible.

Noteahead is written in Qt/QML/C++20 on top of RtMidi back-end + RtAudio for the audio recorder. It builds with CMake and uses CTest + Qt Test framework for unit tests.

<table>
  <tr><td colspan="3"><img src="/screenshots/0.2.0/Noteahead_1.png" width="100%"></td></tr>
  <tr>
    <td><img src="/screenshots/0.6.0/Noteahead_1.png" width="100%"></td>
    <td><img src="/screenshots/0.6.0/Noteahead_2.png" width="100%"></td>
    <td><img src="/screenshots/0.6.0/Noteahead_3.png" width="100%"></td>
  </tr>
  <tr>
    <td><img src="/screenshots/0.15.0/2b.png" width="100%"></td>
    <td><img src="/screenshots/0.15.0/9b.png" width="100%"></td>
    <td><img src="/screenshots/0.15.0/15b.png" width="100%"></td>
  </tr>
</table>

##
## Who is it for?

While it's all about DAWs today, Noteahead is for Linux musicians who enjoy the tracker workflow and want a lightweight, pattern-based MIDI sequencer instead of a full DAW. It’s built for fast, keyboard-driven sequencing of external synths and drum machines, with clear handling of patch changes, automation, and CCs — no cryptic hex values required. If you already use a DAW or mixer for audio, Noteahead can serve as the dedicated MIDI brain of your setup.

My own setup runs Noteahead on Ubuntu 24.04 LTS with all gear connected via USB-MIDI hubs. Synths are routed to an external digital mixer connected to the PC via USB. I record with Noteahead and master in Audacity using LSP (mostly) plugins. I have already produced several songs with it, available on SoundCloud, YouTube Music, and Spotify (see links below) and I'm quite happy with it (see the links below).

##
## Example Tracks

Some example tracks sequenced with Noteahead (produced with Behringer X32 Producer and Audacity):

- [Arctic Music Project - Black Winter (Spotify)](https://open.spotify.com/track/6Au6bJkAxJ5nUYQnoOp9WW)
- [Arctic Music Project - Pointless (YouTube)](https://www.youtube.com/watch?v=xXXgGbOZIV0)
- [Arctic Music Project - Pointless (SoundCloud)](https://soundcloud.com/arctic-music-project/pointless)
- [Arctic Music Project - Why (SoundCloud)](https://soundcloud.com/arctic-music-project/why)
- [Arctic Music Project - Halla Returns (SoundCloud)](https://soundcloud.com/arctic-music-project/halla-returns)
- [Arctic Music Project - Triple Distilled (SoundCloud)](https://soundcloud.com/arctic-music-project/triple-distilled)
- [J.L.P - This Planet (Spotify)](https://open.spotify.com/track/5LV4oR82Ak4uA0kkKB6McX)
- [J.L.P - This Planet - Note Visualization Video (YouTube)](https://www.youtube.com/watch?v=f_Cf_84eXcA)

All Arctic Music Project songs:

- [Arctic Music Project](https://www.arcticmusicproject.com)

##
## Features

### Core / Performance
- **Very accurate internal timing**
  - No jitter, no drift
- Fully scalable UI
- Cool volume meters like in NoiseTracker
- Cool note visualizer animation on the bottom bar
- The Debian package size is currently around 1 MB (is that bloat?)

### Editing
- Easy-to-use track editing
- Tracks with multiple note columns
- Track settings with port, channel, bank, patch, volume, pan, cutoff
  - Can be easily changed on-the-fly via line events
- Track settings with generic MIDI CC slots
  - Initial MIDI CC values can be set directly via Track settings
- Track and column-specific velocity scales
  - Effective velocity is the product of track scale, column scale, and note velocity
- Velocity key track scaling (since 1.4.0)
  - Linearly turn velocity down on higher notes (0-100%)

### MIDI
- Poor Man's MIDI Hot-Plug with automatic setup
  - Noteahead notices when a device goes online/offline
  - Noteahead automatically sets channel, bank, and patch
- Sends MIDI clock pulse and Start/Stop on desired ports
- MIDI side-chain (since 1.2.0)
  - In Track settings the user can select source track and column that triggers the desired MIDI CC event
- MIDI CC automation
  - Select lines and add automation on the desired controller
  - Linear interpolation
  - Sine wave interpolation (since 1.0.0+)
- Pitch Bend automation (linear interpolation)  
  - Select lines and add automation on pitch bend
- Chord automation (since 1.2.0)
  - In Column Settings the user can set offsets for three notes that are relative to the root note
- Step record / play notes via a MIDI controller (since 0.10.0)
  - The MIDI controller is routed to the instrument of the selected track
- Highly adjustable note-off's (global default in ms, per-instrument in ms, manual)
- Virtual MIDI out port (since 1.0.0+)
- Velocity key track
  - Make e.g. your piano have less velocity on higher notes. Super cool!

### Audio
- Audio recorder
  - Just enable recording in `Settings => Audio` and Noteahead will record from the default audio source when the song starts and name the file according to active tracks

### Tools
- Delay time calculator
- Note frequency calculator

### File Formats
- Saves to a custom (but open!) XML-based **.nahd** format
- Exports in **MIDI File Format 1** (SMF Type 1) (since 1.0.0)

### Experimental
- Music video generator (**Experimental**)
  [A note visualization video generated by Noteahead](https://www.youtube.com/watch?v=V--dK6A2FxQ)

##
## Known issues

* The UI can be a bit laggy during playback with Qt older than 6.5.3
  - Unfortunately Ubuntu 24.04 LTS has only Qt 6.4.2 in the repositories
  - However, this doesn't affect the accuracy of MIDI events

##
## Future dreams (**NOT YET IMPLEMENTED**):

* Horizontal visualization
* Sync options: MIDI, Jack

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

Noteahead is currently being developed on Ubuntu 24.04 LTS and on Ubuntu 24.10 with the stock Qt 6 from repositories.

###
### Build dependencies on Ubuntu 24.10+

Packages needed for building:

    $ sudo apt install build-essential cmake pkg-config ninja-build qt6-base-dev qt6-declarative-dev qt6-tools-dev librtmidi-dev librtaudio-dev libsndfile-dev

Additional packages needed to run:

    $ sudo apt install qml6-module-qtcore qml6-module-qtqml qml6-module-qtqml-workerscript qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-layouts  qml6-module-qtquick-templates qml6-module-qtquick-window

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
* **Ctrl + C**: cut the current selection
* **Ctrl + X**: copy the current selection
* **Ctrl + V**: paste the copied selection

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

### Experimental note visualization video generation

Noteahead can be used to generate a note visualization video with particle effects.

To achieve this, these things are needed:

* `ffmpeg` installed (used to render the video from the generated frames)
* A soundtrack file that corresponds with the song (.wav)
* The song file (.nahd)
* Optional background image file (.png)
* Optional logo image file (.png)

An example command to generate a video:

    $ ./noteahead --video-audio Song.wav --video-song Song.nahd --video-scrolling-text Test --video-image Video.png --video-image-zoom-speed 0.0001 --video-logo Logo.png --video-logo-fade-factor 0.99 --video-track-opacity 0.1 --video-lead-in-time 2000 --video-lead-out-time 2000 --video-size 1920x1080

Run `noteahead -h` for all options.

##
## Real-world test cases

I have tested Noteahead with at least these devices (USB MIDI):

* Arturia Keystep
* Behringer DeepMind 6
* Behringer Model D
* Behringer RD-6
* Behringer RD-8 MK II
* Behringer RD-9
* Behringer Solina String Ensemble
* Behringer TD-3-MO
* Behringer VC340
* Eventide SPACE
* Korg Electribe 2
* Korg KROSS
* Korg Minilogue XD
* Korg Monologue
* Novation Bass Station II
* Roland SP-404 MK II
* Roland TR-8S
* Yamaha CP4

##
## Why am I doing this?

First of all, I'm a tracker musician and also a professional software engineer.

1) I don't want to depend on closed source software with closed file formats for my music work.

2) Because I can.

3) Because I want to live in the past.
