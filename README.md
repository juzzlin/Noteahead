# Noteahead

![Build Status](https://github.com/juzzlin/Noteahead/actions/workflows/ci.yml/badge.svg)

Noteahead is a lightweight, pattern-based MIDI sequencer and "half-daw" for Linux, designed for musicians who prefer the tracker workflow but want a modern, self-contained production environment.

While it excels as a dedicated MIDI "brain" for external synthesizers and drum machines, Noteahead now features a powerful suite of internal instruments, a studio-quality virtual rack for mixing, and high-precision rendering capabilities.

Noteahead is written in Qt/QML/C++20 on top of RtMidi back-end + RtAudio. It builds with CMake and uses CTest + Qt Test framework for unit tests.

<table>
  <tr><td colspan="4"><img src="/screenshots/3.1.0/Noteahead.png" width="100%"></td></tr>
  <tr>
    <td><img src="/screenshots/3.1.0/Synth.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/BassSynth.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/DrumSynth.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/Sampler.png" width="100%"></td>
  </tr>
  <tr>
    <td><img src="/screenshots/3.1.0/TrackSettings.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/MidiSettings.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/AudioSettings.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/AudioRenderer.png" width="100%"></td>
  </tr>
  <tr>
    <td><img src="/screenshots/3.1.0/DeviceRack.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/EffectRack.png" width="100%"></td>
    <td><img src="/screenshots/0.6.0/Noteahead_1.png" width="100%"></td>
    <td><img src="/screenshots/0.6.0/Noteahead_3.png" width="100%"></td>
  </tr>
</table>

##
## Who is it for?

While many musicians rely on complex DAWs, Noteahead is for Linux musicians who enjoy the precision and keyboard-driven workflow of trackers but want something more integrated than a simple MIDI sequencer. It is the perfect middle ground—a "half-daw" that can act as the central brain of a MIDI setup or as a standalone production tool using its built-in software synths and effects.

Whether you are sequencing external vintage gear via USB-MIDI or building entire tracks in the box, Noteahead offers a clear interface with studio-quality features like 8-band parametric EQ, feedback delay network reverbs, and a high-precision offline renderer.

My own setup runs Noteahead on Ubuntu 24.04 LTS with all gear connected via USB-MIDI hubs. Synths are routed to an external digital mixer connected to the PC via USB. I record with Noteahead and master in Audacity using LSP (mostly) plugins. I have already produced several songs with it, available on SoundCloud, YouTube Music, and Spotify (see links below).

I now also write songs with the internal instruments only. It has been a huge effort to make this possible.

##
## Example Tracks

Some example tracks sequenced and recorded with Noteahead (usually mastered in Audacity).

A short synth demo:

- [Noteahead Synth Demo 1: I So Threw Up That Day (SoundCloud)](https://soundcloud.com/arctic-music-project/noteahead-synth-demo-1)

A song with Noteahead's built-in synths and drums only:

- [Arctic Music Project - Awesome (Noteahead Remix) (SoundCloud)](https://soundcloud.com/arctic-music-project/awesome-noteahead-remix)

Noteahead with hardware synths:

- [Arctic Music Project - Raindrops (SoundCloud)](https://soundcloud.com/arctic-music-project/raindrops)
- [Arctic Music Project - My Real Name (Spotify)](https://open.spotify.com/track/2WN79Gzazaq0xtji9t0ORI)
- [Arctic Music Project - Epic Success (Epic Piano Remix) (Spotify)](https://open.spotify.com/track/62R7Qb53DbrAatjN6wmzmE)
- [Arctic Music Project - Incredible Times (Spotify)](https://open.spotify.com/track/5mnyB4BaIhdKecEkendF6E)
- [Arctic Music Project - Your Love (Spotify)](https://open.spotify.com/track/1s2hw68GyPfwUfOOkuaGLn)
- [Arctic Music Project - Black Winter (Spotify)](https://open.spotify.com/track/6Au6bJkAxJ5nUYQnoOp9WW)
- [Arctic Music Project - Pointless (YouTube)](https://www.youtube.com/watch?v=xXXgGbOZIV0)
- [Arctic Music Project - Pointless (SoundCloud)](https://soundcloud.com/arctic-music-project/pointless)
- [Arctic Music Project - Why (SoundCloud)](https://soundcloud.com/arctic-music-project/why)
- [Arctic Music Project - 0035AM (Spotify)](https://open.spotify.com/track/4yUJmFjBmUhOTyl8oXu1yU)
- [J.L.P - This Planet (Spotify)](https://open.spotify.com/track/5LV4oR82Ak4uA0kkKB6McX)

All Arctic Music Project songs:

- [Arctic Music Project](https://www.arcticmusicproject.com)

##
## Features

### Core / Performance
- Sample-Accurate Timing
  - Jitter-free and drift-free internal timing strategy.
- High-Precision Offline Renderer
  - Export songs to WAV with sample-accurate timing, preserving all automations and parameters.
- Native Audio Backend Selector
  - Explicit support for **ALSA**, **PulseAudio**, and **JACK** with optional transport synchronization.
- Lightweight & Scalable
  - Fully scalable UI with a Debian package size of around 1 MB.

### Internal Instruments (Virtual Device Rack)
- Virtual Device Rack
  - Central hub for managing multiple independent instances of internal instruments.
- Synth
  - Polyphonic VA synthesizer (up to 6 voices) with dual oscillators, multi-mode filters, ADSR/Mod EGs, LFO, and built-in Delay.
- WavetableSynth
  - 8-voice wavetable synthesizer with two independent wavetable oscillators (Classic and Spectral sets), noise generator, cascaded LPF/HPF filters, Amp and Mod EGs, LFO, and Poly/Unison voice modes with stereo pan spread. Features 2× oversampling and portamento.
- BassSynth
  - Monophonic acid-style synthesizer with sub-oscillator, resonant 24dB LPF, and TB-303 style accent/slide.
- DrumSynth
  - Multi-engine drum machine with 11 independent voices (Kick, Snare, Toms, etc.) and dedicated per-voice controls.
- Sampler
  - 16-pad internal sampler with WAV support, dual filters, and per-sample panning/volume.
- Dynamic Routing
  - Per-device Effect Sends for flexible mixing.

### Master Effects Rack
- Master Effect Rack
  - Studio-quality global effects hub with support for multiple independent instances.
- 8-Band Parametric EQ
  - High-precision equalizer with multiple filter types (Bell, Shelf, Cut, Notch) per band.
- FDN Reverb
  - High-quality Feedback Delay Network algorithm with 8 studio presets (Hall, Cathedral, etc.) and fine-grained controls.
- Compressor
  - Feed-forward compressor with soft-knee interpolation, lookahead support, and real-time gain reduction metering.
- Integrated Effects
  - Includes studio-standard Delay, High-Pass/Low-Pass filters, and Panning/Volume utilities.

### Editing & Sequencing
- Keyboard-Driven Workflow
  - Fast, tracker-style editing using decimal values (0-127).
- Advanced Automation
  - Linear, Sine wave, and Random modulation for MIDI CCs and Pitch Bend.
- Pattern-Based Sequencing
  - Flexible play order management with independent pattern lengths.
- Arpeggiator & Chords
  - Integrated arpeggiator with multiple patterns (Up, Down, Random) and customizable chord offsets.
- MIDI Side-Chain
  - Trigger MIDI CC events based on other tracks or columns.
- Step Recording
  - Record notes directly from a MIDI controller into the editor.

### MIDI & Connectivity
- MIDI Hot-Plug
  - Automatic detection and setup of MIDI devices as they go online/offline.
- Virtual MIDI Out
  - Use Noteahead to control other software or external hardware.
- Wide Hardware Support
  - Tested with a vast range of Arturia, Behringer, Korg, Roland, and Yamaha gear.
- Standard Exports
  - Export to MIDI File Format 1 (SMF Type 1) including automations.

### Tools & Experimental
- Audio Recorder
  - Direct-to-disk recording from the selected audio source.
- Calculators
  - Built-in Delay time and Note frequency calculators.
- Experimental MIDI Import
  - Initial support for importing Standard MIDI Files.

##
## Future dreams (**NOT YET IMPLEMENTED**):

* Horizontal visualization / Piano Roll

##
## Some important design choices

### Software framework

Noteahead is written in Qt/QML/C++20, because it's a relevant, stable, and well-known technology stack especially in the Linux-world. Qt also has a very good support for safe threads and it performs well. The MIDI backend is currently based on RtMidi, but the architecture is such that it can be easily changed.

### Tracks and note columns

Only one "instrument" can be set per track (it's possible to change patch on-the-fly, however). I don't like the traditional concept where each note includes the instrument number and can be inserted anywhere. In my opinion this easily leads into a mess and the workflow is clumsy.

All values are entered in decimal format, typically ranging from 0 to 127 to align with the MIDI protocol and also used in many synthesizers. I've never been a fan of the hexadecimal notation commonly used in trackers.

### Internal timing strategy

The song is rendered into events just before playing. Accurate timestamps are calculated for each event beforehand in order to achieve a drifting-free timing. The player thread syncs to these event timestamps.

### Project file format

Noteahead reads and saves to a custom XML-based format. This has several pros:

* It supports more feature than just a simple MIDI-file.
* A plain text project file works well with version control systems like Git. You can immediately see what you have changed in the project. A binary-formatted project file is a black box and a PITA. 

Several..? That was only two, come on!

##
## License

Noteahead's source code is licensed under **GNU GPLv3**. See COPYING for the complete license text.

##
## Install instructions

Currently Debian packages for Ubuntu 24.04 LTS and Ubuntu 25.10 are provided. Installation, for example:

    $ sudo apt install ./noteahead-0.1.0-ubuntu-24.04_amd64.deb

##
## Build instructions

Currently Noteahead depends on Qt >= 6.4. Actually that's the only thing that defines the Ubuntu version if stock Qt is to be used.

Noteahead is currently being developed on Ubuntu 24.04 LTS and on Ubuntu 25.10 with the stock Qt 6 from repositories.

###
### Build dependencies on Ubuntu (>= 24.04)

Packages needed for building:

    $ sudo apt install build-essential cmake pkg-config ninja-build qt6-base-dev qt6-declarative-dev qt6-tools-dev librtmidi-dev librtaudio-dev libsndfile-dev libjack-jackd2-dev

**Note**: As we are in the middle of the PipeWire transition, the Jack development files might not be needed.

Additional packages needed to run:

    $ sudo apt install qml6-module-qtcore qml6-module-qtqml qml6-module-qtqml-workerscript qml6-module-qtquick qml6-module-qtquick-controls qml6-module-qtquick-dialogs qml6-module-qtquick-layouts qml6-module-qtquick-templates qml6-module-qtquick-window

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
* **Ctrl + X**: cut the current selection
* **Ctrl + C**: copy the current selection
* **Ctrl + V**: paste the copied selection

Transposition (also available via right-clicking on the editor):

* **Alt + F9**: transpose column by -1 semitone
* **Alt + F10**: transpose column by +1 semitone
* **Alt + F11**: transpose column by -12 semitones
* **Alt + F12**: transpose column by +12 semitones
* **Shift + F9**: transpose track by -1 semitone
* **Shift + F10**: transpose track by +1 semitone
* **Shift + F11**: transpose track by -12 semitones
* **Shift + F12**: transpose track by +12 semitones
* **Ctrl + F9**: transpose pattern by -1 semitone
* **Ctrl + F10**: transpose pattern by +1 semitone
* **Ctrl + F11**: transpose pattern by -12 semitones
* **Ctrl + F12**: transpose pattern by +12 semitones

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
