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
- **`devices/`** — The `Device` base, `DrumPadDevice` and `DeviceFactory`. The devices themselves are plugins; see `src/plugins/`.
- **`effects/`** — The `Effect` base, `EffectRack` and `EffectFactory`. The effects themselves are plugins; see `src/plugins/`.
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

### `src/plugins/` — Effects and devices in folders of their own

- **`effects/<name>/`** and **`devices/<name>/`** — Everything one effect or device consists of: its domain C++ (presets and helpers included), any application or view C++ of its own (a device's controller, a preview, a renderer), its `<Name>Dialog.qml` with section files, and its tests, one folder each in `unit_tests/`.
- Each file is still built into the library of its layer, so the layer rules above apply inside a folder as well. The folder's `CMakeLists.txt` is one `noteahead_add_effect()` or `noteahead_add_device()` call listing `DOMAIN_SOURCES`, `APPLICATION_SOURCES`, `VIEW_SOURCES` and `QML_FILES`; the functions are in `src/plugins/CMakeLists.txt`.
- What several effects or devices share stays in its layer: `Effect`, `EffectRack`, `Device`, `DrumPadDevice`, the factories and all of `dsp/`.

### `src/common/` — Shared constants and utilities

- **`constants.hpp/.cpp`** — All XML key strings and application-wide constants. Never hardcode key strings outside this file.

## Data Flow

**Editing**: User input → `EditorService` → `NoteEditCommand` pushed to `UndoStack` → `Song`/`Column`/`NoteData` mutated → `NoteColumnModel` notified → QML ListView re-renders.

**Playback**: `PlayerService` calls `Song::renderToEvents()` → flat `EventList` → `PlayerWorker::initialize()` → `PlayerWorker` thread loops over tick-keyed `EventMap` → `MidiService` sends MIDI / `MixerService` drives internal devices.

**Tracker cell display**: Each cell in `NoteColumnModel` renders as a single formatted string via `DataRole::Line`. Sub-column focus is tracked by `DataRole::LineColumn` (0 = note, 1–3 = velocity digits, 4–5 = delay digits, 6–8 = pan digits; `EditorService::maxLineColumn()` is the last one). The QML `NoteColumn_LineDelegate` uses pixel-offset rectangles to highlight the focused sub-column.

## XML Serialization

- The project file's number formats and compatibility rules are in `DESIGN.md`: integers for anything with a fixed range (10000 for 1 when normalized), decimals only for automation positions and drawn nodes, and never the system's locale.
- Parameter XML keys must be **generic** (no device/effect prefix). E.g., `"lpfCutoff"`, not `"synthLpfCutoff"`.
- When renaming a key, register the old name(s) as legacy names in the `Parameter` constructor's `LegacyNameList`. `ParameterContainer` maps legacy names at deserialization time.
- Every renamed key must have a corresponding legacy-name round-trip test in `xml_serialization_test`.

## Adding a New Rack Effect (checklist)

1. Create `src/plugins/effects/<name>/<name>.hpp/.cpp` inheriting from `Effect`. Implement `type()`, `typeId()`, `typeIdString()` (static), `processSample(double&, double&)` (protected), `sync()` and `reset()`; override `processBlock(AudioContext&)` only for work that cannot be done a frame at a time (lookahead, side-chains, transforms). `Effect::process()` is not virtual: it applies Mix, Solo, bypass and send mode around the effect's own work. Register `Parameter` objects in the constructor, Mix through `addMixParameter(default, MixLaw)` and Solo through `addSoloParameter()`. `Gain` is the smallest template.
2. Add `src/plugins/effects/<name>/CMakeLists.txt` with `noteahead_add_effect(DOMAIN_SOURCES <name>.cpp <name>.hpp QML_FILES <Name>Dialog.qml)`, and `add_subdirectory(effects/<name>)` to `src/plugins/CMakeLists.txt` (alphabetical). New `dsp/` sources go in `src/domain/CMakeLists.txt`; `src/domain/dsp/CMakeLists.txt` builds a library nothing links.
3. Add `xmlKey<Name>()` constant(s) to `Constants::NahdXml` in `src/common/constants.hpp/.cpp`. Add the effect type string to `Constants::RackEffectType`. Reuse existing generic keys where they fit — e.g. `xmlKeyBandGain(i)`, `xmlKeyGain()`.
4. Register the effect in `EffectFactory::init()` in `src/domain/effects/effect_factory.cpp` by `typeIdString()`. The type-string and `registerLegacyEffect()` registrations of older effects exist only for projects saved before type ids; a new effect has none.
5. Add `Q_PROPERTY` type string, `Q_INVOKABLE` parameter-key methods, an `addEffect(...)` line in `availableEffects()`, and an `effectParametersSummary()` branch to `EffectRackController`.
6. Add `<Name>Dialog.qml` to the effect's folder following the *Dialog sizing* rules below (it is listed in step 2's `QML_FILES`), instantiate it in `Main.qml`, and add the branch to `EffectDialogLauncher.qml` — the single place a type is mapped to its dialog.
7. Add the effect's tests in `unit_tests/<name>_test/` in its folder, with their own `CMakeLists.txt`. `noteahead_add_effect()` adds every such folder.
8. Add a round-trip case to `xml_serialization_test`, a gallery case to `effect_rack_controller_test`, and a `CHANGELOG` entry under *New features*.

## Adding a New Device (checklist)

1. Create `src/plugins/devices/<name>/<name>_device.hpp/.cpp` inheriting from `Device`. Implement `name()`, `category()`, `typeName()`, `typeId()`, `typeIdString()` (static), the `processMidi*` methods (MIDI CC arrives through `processDeviceMidiCc()`, not `processMidiCc()`, which `Device` owns so that no device can miss the CCs every device has; advertise the device's own CCs from `deviceMidiCcControllers()`), `processAudio(AudioContext &)`, `hasActiveAudio()`, `reset()`/`resetDeviceAudio()` (the device's own state; `Device::resetAudio()` resets the insert rack and then calls it, so no device can leave its inserts out of a render's reset), `serializeToXml()`/`deserializeFromXml()` (including `serializeMidiMappingsToXml()` and the `<MidiMappings>` branch) and `syncParameters()`. Register `Parameter` objects in the constructor. `PianoSynthDevice` is the smallest complete template.
2. Add `src/plugins/devices/<name>/CMakeLists.txt` with a `noteahead_add_device()` call listing the folder's files under `DOMAIN_SOURCES`, `VIEW_SOURCES` and `QML_FILES`, and `add_subdirectory(devices/<name>)` to `src/plugins/CMakeLists.txt` (alphabetical). New `dsp/` sources go in `src/domain/CMakeLists.txt`; `src/domain/dsp/CMakeLists.txt` builds a library nothing links.
3. Add `<name>DeviceName()` and any new `Constants::NahdXml` keys to `src/common/constants.hpp/.cpp`. Reuse existing generic keys where they fit.
4. Register the device in `DeviceFactory::init()` in `src/domain/devices/device_registration.cpp`.
5. Add `<name>_controller.hpp/.cpp` to the device's folder (one `Q_PROPERTY` per parameter; `int` scaled by `Constants::uiInternalScaling()` for continuous, `bool` for switches), listed under `VIEW_SOURCES`.
6. In `DeviceRackController`: add a `<name>DialogRequested` signal, a branch in `openDevice()`, and an `addDevice(...)` line in `availableDevices()` — which feeds `DeviceGalleryDialog.qml` and is asserted by `device_rack_controller_test`.
7. In `Application`: add the controller member, add it to the `DeviceRackController` controller vector, `qmlRegisterType` it, and expose it as a context property. The dialog takes its title from `<name>Controller.displayName`; the `<name>DeviceName` properties on `ApplicationService` are only for older dialogs.
8. Add `<Name>Dialog.qml` plus its section files to the device's folder, following the *Dialog sizing* rules below, list them under `QML_FILES`, instantiate the dialog in `Main.qml` and connect the request signal in the same `Component.onCompleted` block.
9. Add the device's tests in `unit_tests/<name>_test/` in its folder, with their own `CMakeLists.txt`.
10. Add a project-level round-trip case to `xml_serialization_test` (it covers the factory registration too) and a `CHANGELOG` entry under *New features*.

## Translations

- Translation sources live in `src/translations/noteahead_<locale>.ts`, one per language, checked in.
- Only `lrelease` runs at build time; the `.qm` files are embedded at `:/translations/`. Regenerate
  the sources deliberately with `ninja update_translations`, never as part of a normal build.
- `lupdate` must be the Qt 6 one. On a system with `qtchooser` the `lupdate` on `PATH` is Qt 5's,
  whose QML parser drops strings from files it cannot parse. The CMake target resolves it from the
  `Qt6::lupdate` imported target for this reason.
- Always generate with `-locations none -no-ui-lines -no-obsolete`. Location comments make
  translation merges conflict on every unrelated edit.
- Every user-facing string must be wrapped in `qsTr()` in QML or `tr()` in C++. Never put `qsTr()`
  inside a template literal: Qt 6.4's parser does not extract from those. Use `+` concatenation.
- Adding a language means adding its `.ts` to the `TS` list in `src/CMakeLists.txt` **and** its
  locale to `Constants::Language::supportedLanguages()`. `language_service_test` fails otherwise.
- `QQmlApplicationEngine::retranslate()` only refreshes pure translation bindings. A `model: { ... }`
  JS block containing `qsTr()` must read `languageService.activeLanguage` to declare the dependency,
  and translated strings built in C++ need their notify signal re-emitted from
  `Application::retranslateUi()`.
- Do not translate unit strings (`"0 ms"`, `"12 dB/oct"`), pan sides (`" L"`, `" R"`) or device and
  effect names. Audio jargon conventionally stays in English even in localized DAWs.

## Coding Standards

- C++20; all code in the `noteahead` namespace.
- `const` everywhere applicable. All non-mutating methods must be `const`.
- Smart pointers only; no raw `new`/`delete`.
- `=` for local variable initialization; `{}` for class instantiation only.
- Naming:
  - `PascalCase`: types and enum values.
  - `camelCase`: functions, locals, parameters, plain-struct fields and `constexpr` locals.
  - `m_camelCase`: class data members.
  - `PascalCase`: named constants at namespace or class scope, such as `MaxSweepSeconds`. No `k`
    prefix, no `ALL_CAPS` (except `TAG`) and no `s_`.
  - `snake_case`: C++ file names.
  - Unit-bearing values say their unit: `cutoffHz`, `gainDb`, `decaySeconds`. A bare name means the
    0..1 knob position.
  - Accessors are `cutoff()`/`setCutoff()`, never `getX()`.
- Post-increment (`i++`) for loop counters.
- No string literals duplicated — centralize in `constants.hpp`.
- All implementation in `.cpp`; headers contain declarations only.
- `#ifndef FILENAME_HPP` header guards, not `#pragma once`.
- Use `std::format` for string formatting; use `if`-with-initializer for map lookups.
- Format all C++ with `.clang-format` at the repo root.

## Unit Tests

- Each test in its own subdirectory with its own `CMakeLists.txt`: under the plugin's `unit_tests/` for an effect's or device's own tests, under `src/unit_tests/` for everything else.
- Test headers (`.hpp`): include `<QObject>` only — never `QtTest`.
- Test sources (`.cpp`): use `#include <QTest>` (not `<QtTest>`); add specific headers like `<QSignalSpy>` as needed.
- Do not manually include `.moc` files; rely on CMake's automatic MOC.
- Tests must run headlessly: CI has no display. Use `QTEST_GUILESS_MAIN` unless the test needs a
  `QGuiApplication` (QPainter, fonts, windows); then use `QTEST_MAIN` and force the platform in its
  `CMakeLists.txt` with `set_tests_properties(${NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")`.
- Test function naming: `test_<subject>_<variant>_<condition>` (variant goes after subject, not merged into it).

## QML Dialog Pattern

All dialogs follow the same pattern:
1. Dialog QML file in `src/view/qml/Dialogs/`, or in its plugin folder for an effect or device.
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
