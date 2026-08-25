# Noteahead

![Build Status](https://github.com/juzzlin/Noteahead/actions/workflows/ci.yml/badge.svg)

Noteahead is a pattern-based MIDI sequencer and music production environment for Linux, built around the tracker workflow: fast, keyboard-driven editing with the whole song in front of you.

It is also a complete studio. Ten internal instruments, a virtual device rack with per-device insert effects and sends, a mixer view, 28 rack effects and analyzers, and a sample-accurate offline renderer take a song from the first note to a finished master without ever leaving the application. Point it outwards instead and it drives a room full of MIDI hardware with the same precision — or does both at once, in the same song.

Noteahead is written in Qt/QML/C++20 on top of RtMidi back-end + RtAudio. It builds with CMake and uses CTest + Qt Test framework for unit tests.

<table>
  <tr><td colspan="4"><img src="/screenshots/7.0.0/Noteahead.png" width="100%"></td></tr>
  <tr>
    <td><img src="/screenshots/3.1.0/Synth.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/BassSynth.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/DrumSynth.png" width="100%"></td>
    <td><img src="/screenshots/7.0.0/Sampler.png" width="100%"></td>
  </tr>
  <tr>
    <td><img src="/screenshots/3.1.0/TrackSettings.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/MidiSettings.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/AudioSettings.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/AudioRenderer.png" width="100%"></td>
  </tr>
  <tr>
    <td><img src="/screenshots/6.0.0/DeviceRack.png" width="100%"></td>
    <td><img src="/screenshots/3.1.0/EffectRack.png" width="100%"></td>
    <td><img src="/screenshots/7.0.0/Mixer.png" width="100%"></td>
    <td><img src="/screenshots/0.6.0/Noteahead_3.png" width="100%"></td>
  </tr>
  <tr>
    <td><img src="/screenshots/5.0.0/RTA.png" width="100%"></td>
    <td><img src="/screenshots/5.0.0/LoudnessAnalysisReport.png" width="100%"></td>
    <td><img src="/screenshots/6.0.0/PatternPeek.png" width="100%"></td>
    <td><img src="/screenshots/6.0.0/Theme.png" width="100%"></td>
  </tr>
  <tr><td colspan="4"><img src="/screenshots/7.2.0/Overview.png" width="100%"></td></tr>
</table>

##
## What you can do with it

**Produce entire songs in the box.** Ten internal instruments cover a lot of ground: a six-voice virtual analog Synth, an eight-voice Wavetable Synth, an acid Bass Synth, an eleven-voice Drum Synth, a 16-pad Sampler, two physically modelled pianos, a TR-808-style Kick808, and two vintage string machines after the VP-330/VC340 and the Solina. Each one sits in a slot of the Virtual Device Rack with its own insert effects, sends, fader, level meter and clip LED.

**Mix and master without leaving the application.** All 28 rack effects and analyzers are available both on the master bus and as per-device inserts: four EQs including an 8-band parametric with Mid/Side, two reverbs, single-band and multiband compression, a lookahead limiter, transient shaping, saturation and tube stages, modulation, an auto filter and a phaser — plus LUFS, true-peak and RTA metering to tell you where you stand. The Mixer view puts every device side by side as channel strips.

**Sequence external gear.** Noteahead began as a MIDI brain and is still a very good one. Ports are hot-plugged, every track routes to its own port and channel, and CC and pitch bend automation, an arpeggiator, chords, step recording and MIDI side-chaining are all there. Internal and external instruments live happily in the same song.

**Finish and ship.** The offline renderer writes a master mix or per-track stems to WAV or FLAC with sample-accurate timing, up to 4x oversampling, optional normalization, trim, fade out and tail silence, and a LUFS/LRA/dBTP loudness report. Songs export as SMF Type 1 too, automations included.

### Who it is for

If you already think in patterns and rows rather than in a piano roll, you will be at home in minutes. If you own hardware synths and want a precise, reliable Linux sequencer to drive them, that is exactly where Noteahead started. And if you want neither — just to write music on Linux with open source software and a plain-text project format you can keep in Git — it does that too.

My own setup runs Noteahead on Ubuntu 24.04 LTS with all gear connected via USB-MIDI hubs. Synths are routed to an external digital mixer connected to the PC via USB.

However, these days I write complete songs with nothing but Noteahead's own instruments, which the demos below are there to prove.

##
## Example Tracks

Some example tracks sequenced and recorded with Noteahead (usually mastered in Audacity).

A short synth demo:

- [Noteahead Synth Demo 1: I So Threw Up That Day (SoundCloud)](https://soundcloud.com/arctic-music-project/noteahead-synth-demo-1)

Songs with Noteahead's built-in synths and drums only:

- [Arctic Music Project - Halla Once More (SoundCloud)](https://soundcloud.com/arctic-music-project/halla-once-more)
- [Arctic Music Project - Fairytale (SoundCloud)](https://soundcloud.com/arctic-music-project/fairytale)
- [Arctic Music Project - Massive (SoundCloud)](https://soundcloud.com/arctic-music-project/massive)

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
  - Export songs to WAV or FLAC with sample-accurate timing, preserving all automations and parameters. Master mix or per-track stems, up to 4x oversampling, optional normalization, trim, fade out, tail silence and a LUFS/LRA/dBTP loudness report.
- Native Audio Backend Selector
  - Explicit support for **ALSA**, **PulseAudio**, and **JACK** with optional transport synchronization.
- Scalable Playback Quality
  - Oversampling for realtime playback is selectable (1x/2x/4x) independently of the export, and playback can be spread across CPU cores. Offline rendering always uses all cores.
- Lightweight & Scalable
  - Fully scalable UI with a Debian package size of around 1 MB.
- Themable UI
  - Accent and cursor colors, plus an accent blend that pulls the track and automation palette towards the accent hue. Pattern Peek and the automation drawing style (Tint or Curve) are configurable too.

### Internal Instruments (Virtual Device Rack)
- Virtual Device Rack
  - Central hub for managing multiple independent instances of internal instruments.
  - Per-slot level meter with a gain staging marker, a load readout, and a clip LED that latches on any full-scale output until clicked.
  - Faders run from -inf to +10 dB, with unity three quarters up the throw, on devices and Sampler pads alike.
- Synth
  - Polyphonic VA synthesizer (up to 6 voices) with three oscillators at organ footages (32'..2'), a digital Multi engine, multi-mode filters, ADSR/Mod EGs, two LFOs that can modulate a single oscillator's pitch, Poly/Unison/Dual/Supersaw/Drift/Mono voice modes, and a built-in Delay.
- Wavetable Synth
  - 8-voice wavetable synthesizer with two independent wavetable oscillators (Classic and Spectral sets), noise generator, cascaded LPF/HPF filters, Amp and Mod EGs, LFO, and Poly/Unison voice modes with stereo pan spread. Features 2× oversampling and portamento.
- Bass Synth
  - Monophonic acid-style synthesizer with sub-oscillator, resonant 24dB LPF, and TB-303 style accent/slide.
- Drum Synth
  - Multi-engine drum machine with 11 independent voices (Kick, Snare, Toms, etc.) and dedicated per-voice controls.
- Sampler
  - 16-pad internal sampler with WAV support, dual filters, and per-sample panning/volume.
- Piano Synth
  - Physically modelled (waveguide) piano with brightness, decay, inharmonicity, LPF/HPF shaping, release, and stereo pan spread.
- Piano Synth V2
  - A larger physically modelled piano, voiced against a Yamaha CP88 recording. A bank of resonators, one per partial, so that stiffness, per-partial decay, the two-stage decay of a struck unison and the weakly radiated bass fundamental are each modelled on their own. Hammer Hardness, Richness, Brightness, Inharmonicity, Unison Detune, Double Decay and Stretch Tuning.
- Kick808
  - TR-808-style bass drum built on a pulse-excited resonator, sweeping from a short click to a long sub boom. Pitched and monophonic, so Key Track and Glide let it play bass lines as well as drums, with Tuning, Tone, Decay, Drive and a pitch envelope.
- String & Voice
  - Vintage string/choir ensemble inspired by the Roland VP-330 / Behringer VC340, with strings and voice registers, BBD ensemble chorus, and a sidechain vocoder.
- String Ensemble
  - Divide-down string machine in the Solina tradition: twelve master oscillators tapped at 16'/8'/4' by every key, so octaves stay phase-locked and polyphony is unlimited. Bass section (Contrabass, Cello) below the split with its own Volume Bass, upper section (Horn, Viola, Trumpet, Violin) above it, plus Crescendo/Sustain Length, ensemble chorus and a swept phaser.
- Sub Mixer
  - Groups other devices so a whole set is mixed and processed as one entity, with its own insert effects, Volume, Gain and Pan. Members keep their own reverb sends, Sub Mixers can be nested, and Volume/Pan can be ridden from a track over MIDI CC.
- Dynamic Routing
  - Per-device Effect Sends for flexible mixing.
- Mixer View
  - Every device side by side as channel strips, with the same Gain, Fader and Pan as the device's own dialog, its meter and clip LED, and shortcuts to the device, its insert rack and its sends.
- Song Overview
  - A signal-flow map of the whole project, drawn as the engine actually runs it. Each device's chain is laid out in its real order, so a device whose fader sits after its inserts reads differently from one whose fader comes first, and sends leave from the tap they are taken at, labelled pre- or post-fader. Hovering a device lights its whole route to the master and dims the rest; clicking a box opens that device's editor.

### Effects Racks
- Master and Per-Device Effect Racks
  - Studio-quality effects hub with support for multiple independent instances. Every effect below is available in both the master rack and each device's insert rack.
- 8-Band Parametric EQ
  - High-precision equalizer with multiple filter types (Bell, Shelf, Cut, Notch) per band and Mid/Side processing modes.
- Vintage Passive EQ
  - Passive "program equalizer" in the EQP-1A tradition, with interacting low Boost/Atten shelves, a high bell Boost with Bandwidth, and a separate high Atten shelf.
- Air Band EQ
  - Six-band parallel-summed "air band" equalizer whose fixed bands (Sub, 40 Hz, 160 Hz, 650 Hz, 2.5 kHz) are taps added back to the dry signal rather than a cascade.
- Simple EQ
  - A single "Sounds Good" knob sweeping a fixed loudness-smile curve: low-shelf body, a gentle low-mid scoop and a high-shelf air lift.
- FDN Reverb
  - High-quality Feedback Delay Network algorithm with 8 studio presets (Hall, Cathedral, etc.) and fine-grained controls.
- Endless Reverb
  - Large ambient reverb built on an 8-line modulated Householder FDN with input diffusion, plus a Freeze switch for an infinite tail.
- Compressor
  - Feed-forward compressor with soft-knee interpolation, lookahead support, Peak/RMS detection, side-chain source selection, and real-time gain reduction metering.
- Multiband Compressor
  - Three bands split by Linkwitz-Riley crossovers that sum back flat, with per-band threshold, ratio, knee, attack, release, makeup, bypass, solo and gain reduction metering. The side chain source is split by the same crossovers.
- Auto Ducker
  - Side-chain driven level rider with Threshold, Knee, Attack, Hold and Release. Amount is signed, so the same effect ducks the signal out of the way or lifts it with the side chain.
- Limiter
  - Lookahead brickwall peak limiter with Threshold, Ceiling, Release and Lookahead controls, a Boost switch for maximum loudness, and gain reduction metering.
- Clipper
  - Hard/soft clipper with adjustable threshold and output gain.
- Saturator
  - Multi-mode saturation/distortion for adding harmonics and warmth.
- Tube Stage
  - Valve preamp stage with a Bias control for the operating point, so the same drive can run from nearly symmetric to hard against cutoff, plus Triode/Pentode curves, a tilt Tone control and oversampled shaping.
- Drive
  - Overdrive with Drive amount, dry/wet Mix and output Gain, in Soft (tanh), Hard (clip), Fold (wavefolder) and Dist algorithms.
- Bass Grinder
  - Split-band bass preamp distortion: Split keeps the fundamental out of an asymmetric diode clipper so a kick or a bass stays weighty while the band above it is ground up, with Blend, Drive, a Color voicing and a three-band tone stack.
- Wave Designer
  - Transient shaper with Attack and Sustain, level-independent by design, plus Gain, Mix and a gain meter. Not a compressor: it acts on how sharply the signal changes, never on how loud it is.
- Stereo Exciter
  - Aural exciter that distorts the band above Tune and adds the harmonics back, for air and detail that was never in the signal. Odd/even Timbre blend, Harmonics, Mix and a Solo mode.
- Stereo Enhancer
  - Psychoacoustic "psycho EQ" with saturated bass harmonics, a midrange dip, a top band and a side-only Spread that leaves a mono source mono.
- Chorus
  - Stereo chorus with rate, depth, delay, width, and LPF/HPF shaping.
- Delay
  - Studio-standard delay with feedback, tempo sync, and Mono/Ping-Pong/Tape modes.
- Auto Filter
  - Cutoff and resonance sweeps that belong to the rack rather than to the instrument. LPF/HPF/BPF/Notch at 12 or 24 dB/oct, two LFOs with the Synth's waveforms and tempo sync (one on the cutoff, one on the resonance), an envelope follower with attack and release, bipolar non-linear intensities, a stereo phase offset for wide sweeps, gain and mix.
- Phaser
  - 2 to 12 all-pass stages, one notch per pair, swept by an LFO with the Synth's waveforms and tempo sync. Centre frequency, Depth, bipolar Feedback, a stereo phase offset defaulting to quadrature, gain and mix, plus a Rate Divider (1 to 64) for sweeps lasting minutes or several bars.
- Panner & Auto Panner
  - Static stereo panning plus an LFO-driven auto panner.
- All-Pass Filter
  - Multi-stage all-pass filter for phase shaping.
- Metering & Analysis
  - LUFS loudness meter with a live gated integrated reading, dBTP true-peak meter, and a real-time analyzer (RTA).
- Integrated Effects
  - Includes High-Pass/Low-Pass filters and Panning/Volume utilities.

### Editing & Sequencing
- Keyboard-Driven Workflow
  - Fast, tracker-style editing using decimal values (0-127).
- Advanced Automation
  - Linear, Sine wave, and Random modulation for MIDI CCs and Pitch Bend.
- Pattern-Based Sequencing
  - Flexible play order management with independent pattern lengths.
- Track & Column Management
  - Move tracks and note columns left or right, wrapping around the ends, and delete a column. Deleting is a soft delete: adding a column back restores the most recently deleted one with its notes, automations and mixer settings, and the columns left behind keep their own.
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
  - Built-in Delay time, Note frequency and dB-to-linear Gain converters.
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

Currently Debian packages for Ubuntu 24.04 LTS and Ubuntu 26.04 LTS are provided. Installation, for example:

    $ sudo apt install ./noteahead-0.1.0-ubuntu-24.04_amd64.deb

On other distributions, use the AppImage. It carries its own Qt and needs nothing installed
beyond the graphics and audio libraries a desktop already has. JACK is optional: a copy is
bundled and used only if the system has none, so the system's own JACK is always preferred
when it is there.

    $ chmod +x Noteahead-7.0.0-x86_64.AppImage
    $ ./Noteahead-7.0.0-x86_64.AppImage

It requires glibc 2.38 or newer, which covers Ubuntu 24.04, Debian 13 and Fedora 39 onwards.

To build one yourself, run `./scripts/build-appimage-docker` in the project root. It builds in an
Ubuntu 24.04 container, so the AppImage stays portable whatever the host happens to be. There is
also `./scripts/build-appimage`, which builds directly on the host: quicker to iterate on, but the
result only runs on hosts at least as new as the one that built it.

##
## Build instructions

Currently Noteahead depends on Qt >= 6.4. Actually that's the only thing that defines the Ubuntu version if stock Qt is to be used.

Noteahead is currently being developed on Ubuntu 24.04 LTS and on Ubuntu 26.04 LTS with the stock Qt 6 from repositories.

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
## User manual

Noteahead ships with a complete user manual, available in the application under **Help => User Manual...**. It has a table of contents, follows the current theme, and covers everything in detail: the toolbar, the editor columns, every internal device and effect, automations, synchronization, and audio rendering.

The manual source lives in [`src/view/qml/Manual.html`](src/view/qml/Manual.html).

A quick keyboard shortcut reference is also available under **Help => Shortcuts...**, and **Help => What's New...** shows what changed in the current version.

The section below is only a quick start — the manual is the authoritative reference.

##
## Basic usage

### Quick start

**1) Give a track a sound.** Click the settings icon on a track header to open **Track Settings**. A track plays either an external MIDI instrument or one of Noteahead's internal devices:

* **External MIDI**: pick the MIDI output port and channel of your synth or drum machine. Devices are hot-plugged, so they appear as they come online.
* **Internal device**: click **Device Rack...** in the same dialog (also under **Devices => Device rack...**), press **(+)** on a free slot, pick an instrument from the **Device Gallery** — Synth, Wavetable Synth, Bass Synth, Drum Synth, Sampler, Piano Synth, Piano Synth V2, Kick 808, String & Voice, String Ensemble or Sub Mixer — and then select its internal port (e.g. *Noteahead Synth 1*) back in Track Settings. Nothing external is needed; Noteahead renders these itself.

**2) Set up the track.** Click the track name to rename it. `[+]`/`[-]` in the track header adds or removes note columns.

**3) Sequence.** Press **ESC** to enter edit mode and use your PC keyboard as a virtual "piano": **Z**..**M** is the lower octave (**Z** is "C"), **Q**..**U** the higher one. The **STEP**, **VEL** and **OCT** spinners in the toolbar control the cursor advance, the default velocity and the base octave.

**4) Play.** Press **SPACE** or use the play buttons.

**5) Build the song.** In the *Pattern* section, the **PAT** spinner selects the pattern being edited (increase it to create a new one) and **LEN** sets its length in lines — each pattern can have its own. Click the pattern name to rename it. In the *Song* section, **POS** selects the position in the play order, **PAT** the pattern assigned to it, and **LEN** the song length. Right-click a position button to mark it as skipped.

**6) Mix.** Each device rack slot has **Insert FX** for its own effect rack, **Sends** for the shared send effects, and a fader with a level meter and clip LED for gain staging. **Devices => Mixer...** shows every device side by side as channel strips for balancing the whole thing at once. The master rack is under **Effects => Master effect rack...**.

**7) Export.** **File => Render audio...** renders offline to WAV or FLAC, either as a master mix or as per-track stems, with optional normalization and a loudness report. **File => Export MIDI file...** writes an SMF Type 1 file including automations.

### Editor columns

Each note column line has four fields:

    C-4 100 00 064

* **Note**: pitch and octave, or `OFF` for a note off event.
* **Velocity**: `000`..`127`.
* **Delay**: sub-line timing offset for humanizing.
* **Pan**: `000`..`127`, sent as MIDI CC #10, on any note column and on any line, with or without a note. MIDI CC #10 is channel-wide, so if several columns carry a pan value on the same line their average is what gets sent — for independently panned parts use separate tracks or the Panner rack effect.

### Context menu

The main context menu is accessed by right-clicking on the editor view. Its actions are grouped by scope — **Line**, **Column**, **Track**, **Pattern**, **Song** and **Selection** — and cover cut/copy/paste, transposition, note-off insertion, velocity and pan interpolation, MIDI CC and pitch bend automations, and line events such as patch changes. Lines that have an event assigned are rendered in an accent color.

### Most important "special" keys

| Key | Action |
| --- | --- |
| **ESC** | Toggle edit mode |
| **SPACE** | Toggle play mode |
| **INSERT** | Insert an empty line and move subsequent lines down |
| **BACKSPACE** | Delete the current line and pull subsequent lines up |
| **DELETE** | Clear the current event |
| **A** | Insert a note off event |
| **F3** / **F4** | Decrease / increase the current octave |
| **Z**..**M** | Play/insert notes of the lower octave |
| **Q**..**U** | Play/insert notes of the higher octave |

Cut/Copy/Paste (also available via right-clicking on the editor):

| Scope | Cut | Copy | Paste |
| --- | --- | --- | --- |
| Selection | **Ctrl + X** | **Ctrl + C** | **Ctrl + V** |
| Column | **Alt + F3** | **Alt + F4** | **Alt + F5** |
| Track | **Shift + F3** | **Shift + F4** | **Shift + F5** |
| Pattern | **Ctrl + F3** | **Ctrl + F4** | **Ctrl + F5** |

Transposition (also available via right-clicking on the editor):

| Scope | -1 semitone | +1 semitone | -12 semitones | +12 semitones |
| --- | --- | --- | --- | --- |
| Column | **Alt + F9** | **Alt + F10** | **Alt + F11** | **Alt + F12** |
| Track | **Shift + F9** | **Shift + F10** | **Shift + F11** | **Shift + F12** |
| Pattern | **Ctrl + F9** | **Ctrl + F10** | **Ctrl + F11** | **Ctrl + F12** |

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
