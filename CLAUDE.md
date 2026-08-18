# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

# Claude Instructions for Noteahead

Noteahead is a MIDI tracker and sequencer for Linux, written in **Qt/QML** and **C++20**.

## Build & Test

- **Initial configure**: `cd build && cmake -GNinja ..` (from repo root: `cmake -GNinja -B build`)
- **Build**: `cmake --build build`
- **Run tests**: `ctest --test-dir build`
- **Run a single test**: `ctest --test-dir build -R <test_name>` (e.g., `-R xml_serialization_test`)
- **Create Debian package**: `cd build && cpack -G DEB`

Always build and run tests after making changes. Fix all failures before reporting done.

## Architecture

The codebase is split into five layers. Logic must never leak upward (domain knows nothing about application or view).

### `src/domain/` — Pure business logic, no Qt/UI dependencies

- **`tracker/`** — Core data model: `Song` → `Track` → `Pattern` → `Column` → `Line` → `NoteData`. Also: `Event`, `ColumnSettings`, `Instrument`, `InstrumentSettings`, `Interpolator`, `PlayOrder`, `Arpeggiator`.
- **`devices/`** — Internal virtual instruments (`SynthDevice`, `BassSynthDevice`, `DrumSynthDevice`, `WavetableSynthDevice`, `SamplerDevice`, `PianoSynthDevice`). Registered via `DeviceFactory`.
- **`effects/`** — Base `Effect`/`EffectRack` plus simple rack effects (panner, volume, delay, HPF/LPF, auto-panner). Registered via `EffectFactory`.
- **`dsp/`** — Heavy DSP components (chorus, clipper, compressor, EQ, reverb, ADSR, LFO, filters, oscillators).
- **`midi/`** — MIDI data types (`MidiCcData`, `PitchBendData`, `MidiNoteData`).

### `src/application/` — Orchestration; bridges domain and view

- **`service/`** — High-level services: `EditorService` (editing, undo/redo), `PlayerService`/`PlayerWorker` (playback thread), `MidiService`, `MixerService`, `AudioService`, `RenderService`, `SelectionService`, `AutomationService`, `SideChainService`, etc.
- **`models/`** — `QAbstractListModel` subclasses for QML: `NoteColumnModel` (tracker rows), `TrackSettingsModel`, `ColumnSettingsModel`, `MidiCcAutomationsModel`, etc.
- **`command/`** — Undo/Redo via `Command` interface, `CompositeCommand`, `NoteEditCommand`, `AutomationCommand`, `UndoStack`. All note edits in `EditorService` push commands here.

### `src/infra/` — External system adapters

- **`xml/`** — `ProjectReader`/`ProjectWriter` wrapping Qt XML. Serialization entry points for `Song`.
- **`midi/`** — RtMidi backend, MIDI file export/import.
- **`audio/`** — RtAudio/JACK backend, audio recording.

### `src/view/` — Pure presentation; no logic

- **`controllers/`** — Thin `QObject` bridges exposing domain services to QML via `Q_INVOKABLE` and `Q_PROPERTY` (e.g., `EffectRackController`, `DeviceRackController`, `SynthController`). Logic stays in services; controllers only translate.
- **`qml/`** — All QML. `Main.qml` is the root. `UiService.qml` is a QML singleton that orchestrates dialog visibility via signals — all dialog open requests flow through it. Dialogs live in `qml/Dialogs/`. The tracker editor lives in `qml/Editor/`.

### `src/common/` — Shared constants and utilities

- **`constants.hpp/.cpp`** — All XML key strings and application-wide constants. Never hardcode key strings outside this file.

## Data Flow

**Editing**: User input → `EditorService` → `NoteEditCommand` pushed to `UndoStack` → `Song`/`Column`/`NoteData` mutated → `NoteColumnModel` notified → QML ListView re-renders.

**Playback**: `PlayerService` calls `Song::renderToEvents()` → flat `EventList` → `PlayerWorker::initialize()` → `PlayerWorker` thread loops over tick-keyed `EventMap` → `MidiService` sends MIDI / `MixerService` drives internal devices.

**Tracker cell display**: Each cell in `NoteColumnModel` renders as a single formatted string via `DataRole::Line`. Sub-column focus is tracked by `DataRole::LineColumn` (0 = note, 1–3 = velocity digits, 4–5 = delay digits, 6–8 = pan digits; `EditorService::maxLineColumn()` is the last one). The QML `NoteColumn_LineDelegate` uses pixel-offset rectangles to highlight the focused sub-column.

## XML Serialization

- Parameter XML keys must be **generic** (no device/effect prefix). E.g., `"lpfCutoff"`, not `"synthLpfCutoff"`.
- When renaming a key, register the old name(s) as legacy names in the `Parameter` constructor's `LegacyNameList`. `ParameterContainer` maps legacy names at deserialization time.
- Every renamed key must have a corresponding legacy-name round-trip test in `xml_serialization_test`.

## Adding a New Rack Effect (checklist)

1. Create `src/domain/effects/<name>.hpp/.cpp` inheriting from `Effect`. Implement `type()`, `typeId()`, `typeIdString()` (static), `process(double&, double&)`, `process(AudioContext&)`, `sync()`, `reset()`. Register `Parameter` objects in the constructor.
2. Add both files to `HEADER_FILES` and `SOURCE_FILES` in `src/domain/CMakeLists.txt` (alphabetical). New `dsp/` sources also go in `src/domain/dsp/CMakeLists.txt`.
3. Add `xmlKey<Name>()` constant(s) to `Constants::NahdXml` in `src/common/constants.hpp/.cpp`. Add the effect type string to `Constants::RackEffectType`. Reuse existing generic keys where they fit — e.g. `xmlKeyBandGain(i)`, `xmlKeyGain()`.
4. Register the effect in `EffectFactory::init()` in `src/domain/effects/effect_factory.cpp`: by `typeIdString()`, by type string, and a `registerLegacyEffect()` snake_case alias.
5. Add `Q_PROPERTY` type string, `Q_INVOKABLE` parameter-key methods, an `addEffect(...)` line in `availableEffects()`, and an `effectParametersSummary()` branch to `EffectRackController`.
6. Add `src/view/qml/Dialogs/<Name>Dialog.qml` following the *Dialog sizing* rules below, register it in `QML_SOURCE_FILES` in `src/CMakeLists.txt` (alphabetical), instantiate it in `Main.qml`, and add the click handler to **both** `MasterEffectsDialog.qml` and `DeviceInsertEffectsDialog.qml`.
7. Add `src/unit_tests/<name>_test/` with its own `CMakeLists.txt`, plus `add_subdirectory` in `src/unit_tests/CMakeLists.txt`.
8. Add a round-trip case to `xml_serialization_test` and a `CHANGELOG` entry under *New features*.

## Adding a New Device (checklist)

1. Create `src/domain/devices/<name>_device.hpp/.cpp` inheriting from `Device`. Implement `name()`, `category()`, `typeName()`, `typeId()`, `typeIdString()` (static), the `processMidi*` methods, `processAudio(AudioContext &)`, `hasActiveAudio()`, `reset()`/`resetAudio()`, `serializeToXml()`/`deserializeFromXml()` and `syncParameters()`. Register `Parameter` objects in the constructor. `PianoSynthDevice` is the smallest complete template.
2. Add both files to `HEADER_FILES` and `SOURCE_FILES` in `src/domain/CMakeLists.txt` (alphabetical). New `dsp/` sources go in **both** `src/domain/CMakeLists.txt` and `src/domain/dsp/CMakeLists.txt`.
3. Add `<name>DeviceName()` and any new `Constants::NahdXml` keys to `src/common/constants.hpp/.cpp`. Reuse existing generic keys where they fit.
4. Register the device in `DeviceFactory::init()` in `src/domain/devices/device_registration.cpp`.
5. Add `src/view/controllers/<name>_controller.hpp/.cpp` (one `Q_PROPERTY` per parameter; `int` scaled by `Constants::uiInternalScaling()` for continuous, `bool` for switches) and both files to `src/view/CMakeLists.txt`.
6. In `DeviceRackController`: add a `<name>DialogRequested` signal, a branch in `openDevice()`, and an `addDevice(...)` line in `availableDevices()` — which feeds `DeviceGalleryDialog.qml` and is asserted by `device_rack_controller_test`.
7. In `Application`: add the controller member, add it to the `DeviceRackController` controller vector, `qmlRegisterType` it, and expose it as a context property. Add a `<name>DeviceName` property to `ApplicationService` for the dialog title.
8. Add `src/view/qml/Dialogs/<Name>Dialog.qml` plus its section files, following the *Dialog sizing* rules below, register them in `QML_SOURCE_FILES` in `src/CMakeLists.txt` (alphabetical), instantiate the dialog in `Main.qml` and connect the request signal in the same `Component.onCompleted` block.
9. Add `src/unit_tests/<name>_test/` with its own `CMakeLists.txt`, plus `add_subdirectory` in `src/unit_tests/CMakeLists.txt`.
10. Add a project-level round-trip case to `xml_serialization_test` (it covers the factory registration too) and a `CHANGELOG` entry under *New features*.

## Coding Standards

- C++20; all code in the `noteahead` namespace.
- `const` everywhere applicable. All non-mutating methods must be `const`.
- Smart pointers only; no raw `new`/`delete`.
- `=` for local variable initialization; `{}` for class instantiation only.
- Post-increment (`i++`) for loop counters.
- No string literals duplicated — centralize in `constants.hpp`.
- All implementation in `.cpp`; headers contain declarations only.
- `#ifndef FILENAME_HPP` header guards, not `#pragma once`.
- Use `std::format` for string formatting; use `if`-with-initializer for map lookups.
- Format all C++ with `.clang-format` at the repo root.

## Unit Tests

- Each test in its own subdirectory under `src/unit_tests/` with its own `CMakeLists.txt`.
- Test headers (`.hpp`): include `<QObject>` only — never `QtTest`.
- Test sources (`.cpp`): use `#include <QTest>` (not `<QtTest>`); add specific headers like `<QSignalSpy>` as needed.
- Do not manually include `.moc` files; rely on CMake's automatic MOC.
- Test function naming: `test_<subject>_<variant>_<condition>` (variant goes after subject, not merged into it).

## QML Dialog Pattern

All dialogs follow the same pattern:
1. Dialog QML file in `src/view/qml/Dialogs/`.
2. Instantiated (lazily or eagerly) in `Main.qml`.
3. Opened by connecting to a signal on `UiService.qml` (the singleton).
4. `UiService` exposes `Q_INVOKABLE` request methods called from controllers or other QML.

Never open a dialog directly from a controller — always route through `UiService`.

### Dialog sizing

The application runs at 1024x768 at the smallest (`Constants.minWindowWidth/minWindowHeight`), so a dialog must fit that. Rules:

- Size the dialog in **its own file**, as a fraction of the window: `width: parent ? parent.width * Constants.largeDialogScale : <design width>`. Never repeat the size in `Main.qml` — the instantiation there only sets `anchors.centerIn`.
- Put anything that can grow taller than the dialog in a `ScrollView` (`clip: true`, `ScrollBar.vertical.policy: AsNeeded`, `ScrollBar.horizontal.policy: AlwaysOff`), and size its content by `width: <view>.availableWidth`.
- A child that must keep its size inside a layout has to say so: without `Layout.minimumHeight` the layout takes the whole deficit of a short dialog out of it. `VirtualKeyboard` declares its own.
- `AnimatedDialog` sets `clip: true` for everyone, so anything still too big is cut inside the dialog instead of painted over the window. Don't repeat it per dialog.

## Thread Safety

`PlayerWorker` runs in a dedicated thread. Communicate with it only via Qt signals/slots with `Qt::QueuedConnection`, or under the mutex guarded in `PlayerWorker`. Never access domain objects directly from the UI thread while playback is running.

## Git

- Imperative tense in commit messages ("Add", "Fix", "Update" — not "Added", "Fixed").
- First line under 50 characters.
