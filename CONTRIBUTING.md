# Contributing to Noteahead

Noteahead is a MIDI tracker and sequencer for Linux, written in C++20 and Qt 6/QML. This guide
covers the workflow and conventions, and then walks through the two most common kinds of
contribution in detail: a new **rack effect** and a new **internal device** (an instrument, a
mixer or a modulator in the Virtual Device Rack).

The quickest way to learn the shape of a change is to read a recent one end to end:

- An effect: `git show --stat $(git log --format=%h -1 --grep="Add Reverb 224")`
- A device: `git show --stat $(git log --format=%h -1 --grep="Add Kick 909")`

Every file those commits touch is listed below, with what goes into it.

## Building and testing

```sh
cmake -GNinja -B build          # configure once
cmake --build build             # build
ctest --test-dir build          # run every unit test
ctest --test-dir build -R <name> # run one, e.g. -R xml_serialization_test
```

Build and run the whole test suite before sending a change. CI has no display, so every test has
to run headlessly (see [Unit tests](#unit-tests)).

## Architecture in one paragraph

The code is in five layers, and logic never leaks upward:

| Layer | Contents |
|---|---|
| `src/domain/` | Pure logic: the tracker model, devices, effects, DSP. No UI. |
| `src/application/` | Services that orchestrate the domain, QML list models, undo/redo commands. |
| `src/infra/` | Adapters: XML, MIDI, audio backends. |
| `src/view/` | Thin `QObject` controllers for QML, and all QML. No logic. |
| `src/common/` | Constants, including every XML key string, and small utilities. |

A device or an effect lives in `domain/`. QML reaches it only through a controller in
`view/controllers/`, and dialogs are opened only through the `UiService` QML singleton or a
controller signal connected in `Main.qml`. A controller never opens a dialog itself.

## Coding conventions

- C++20, everything in the `noteahead` namespace.
- `const` wherever it applies; every non-mutating method is `const`.
- Smart pointers only, no raw `new`/`delete`.
- `=` to initialise local variables, `{}` to instantiate classes.
- Post-increment (`i++`) in loops.
- Implementation in `.cpp`; headers hold declarations only.
- `#ifndef FILE_NAME_HPP` header guards, not `#pragma once`.
- `std::format` for string formatting; `if` with an initializer for map lookups.
- Prefer `std::optional` to sentinel values such as `-1`.
- No string literal is repeated: XML keys and other shared strings live in
  `src/common/constants.hpp/.cpp`.
- Format all C++ with the `.clang-format` at the repository root:
  `clang-format -i <files>`.
- Comments explain *why*, not *what*. A `//!` doc comment on a class or method says what it means
  to its caller.

### Naming

| What | Style | Example |
|---|---|---|
| Classes, structs, enums, type aliases | `PascalCase` | `Kick909Device`, `enum class Mode`, `using DeviceS` |
| Enum values | `PascalCase` | `Mode::Relative`, `Waveform::Sine` |
| Functions and methods | `camelCase` | `processSample()`, `lpfCutoff()`, `setLpfCutoff()` |
| Local variables and parameters | `camelCase` | `sampleRate`, `cutoffHz` |
| Data members of classes | `m_` + `camelCase` | `m_lpfCutoff`, `m_voices` |
| Fields of plain structs | `camelCase`, no prefix | `Target::releaseValue` |
| Named constants (namespace or class scope) | `PascalCase` | `DefaultPitchHz`, `MaxSweepSeconds`, `RootNote` |
| `constexpr` locals inside a function | `camelCase` | `constexpr double floorGain = ...` |
| The logging tag | `TAG` | `static const auto TAG = "MidiService";` |
| Namespaces | lowercase | `noteahead`, `noteahead::ParameterModulation` |
| C++ files and test directories | `snake_case` | `kick_909_device.cpp`, `kick_909_test/` |
| QML files | `PascalCase`, sections after `_` | `Kick909Dialog.qml`, `Kick909Dialog_Global.qml` |
| QML ids and properties | `camelCase` | `id: kick909Dialog`, `property int effectIndex` |
| Context properties for controllers | `camelCase` | `kick909Controller` |
| XML keys | `camelCase`, no device prefix | `lpfCutoff`, made by `xmlKeyLpfCutoff()` |

Some rules on top of the styles:

- No `k` prefix, no `ALL_CAPS` constants, and no `s_` statics. `TAG` is the one exception.
- A value in a unit says so in its name: `cutoffHz`, `gainDb`, `decaySeconds`, `delayMs`,
  `pitchSemitones`. A bare name such as `cutoff` means the 0..1 knob position.
- Accessors are named after the property: `cutoff()` and `setCutoff()`, not `getCutoff()`.
  Booleans read as a statement: `isActive()`, `hasActiveAudio()`, `supportsOctave()`.
- Name a setting after what it turns on, not after the default it keeps. For example, use
  `chromatic` rather than `fixedPitch`.
- Spell words out. Short forms are fine where the domain uses them: `lfo`, `lpf`, `hpf`, `adsr`,
  `cc`.

## Parameters and saved projects

Devices and effects both keep their settings as `Parameter` objects in a `ParameterContainer`,
and both are serialized from them. A few rules keep old projects loading as they were saved:

- **Never break an existing project.** A new parameter has to default to the old behaviour, so a
  project saved before it existed sounds the same when loaded after.
- **XML keys are generic.** Name a key after what it is, with no device or effect prefix:
  `lpfCutoff`, not `synthLpfCutoff`. Reuse an existing key from `Constants::NahdXml` wherever one
  fits, such as `xmlKeyGain()`, `xmlKeyBandGain(i)` or `xmlKeyLpfCutoff()`.
- **Renaming a key** means listing the old name in the `LegacyNameList` argument of the
  `Parameter` constructor, and adding a legacy-name round-trip test to `xml_serialization_test`.
- **A persisted enum is append-only.** Its ordinals are saved in project files.

The `Parameter` constructor is:

```cpp
Parameter(const std::string & name, float internalValue, int xmlMin, int xmlMax, int xmlDefault,
          int xmlScale = 1, Type type = Type::Continuous, LegacyNameList legacyNames = {},
          LegacyValueConverter legacyValueConverter = {});
```

A continuous parameter runs 0..1 inside the program. `xmlMin`/`xmlMax`/`xmlScale` set how it is
written to XML. A common shape is `{ key, 0.5f, 0, 10000, 5000, 100 }`. A bipolar control is still
0..1 inside, centred at 0.5, so read it as `value * 2 - 1`.

Give each continuous parameter a **display description** (unit, mapping, label) through
`DeviceParameterDisplay` or `setParameterDisplay()`. Automation, the LFO device and the XY Pad
offer only continuous parameters that have one. The `DeviceParameterDisplay` helpers cover the
usual cases:

- `percent(...)` and `bipolar(...)` for plain knobs.
- `filters(*this)` for the standard LPF/HPF pair.

---

## Adding a rack effect

Effects are processed in insert racks (per device), on send buses and on the master bus. The
`Effect` base class applies everything every effect shares: Mix, Solo, enable/bypass, send mode and
output gain. An effect implements only its own processing.

### 1. The effect class: `src/domain/effects/<name>.hpp/.cpp`

Inherit from `Effect` and implement:

| Method | Purpose |
|---|---|
| `static std::string typeIdString()` | A fresh UUID that never changes. It is what a project saves. |
| `std::string typeId() const override` | Returns `typeIdString()`. |
| `std::string type() const override` | Returns `Constants::RackEffectType::<name>()`, which the view uses to pick the dialog and the summary. |
| `void processSample(double & left, double & right) override` | The effect's work on one frame (protected). |
| `void processBlock(AudioContext & context) override` | Optional (protected). For work that cannot be done a frame at a time, such as lookahead, side-chains or transforms. The default loops `processSample()`. |
| `void sync() override` | Reads the parameters into the members the DSP runs on. Called after any parameter change. |
| `void reset() override` | Clears DSP state: delay lines, envelopes, filter memories. |

`Effect::process(...)` is deliberately not virtual: do not override it.

Register the parameters in the constructor. Use the base class helpers for the shared controls:

- `addMixParameter(default, MixLaw)`:
  - `MixLaw::Crossfade` for something that reshapes the signal.
  - `MixLaw::Additive` for a reverb or delay, whose dry has to stay whole.
  - `MixLaw::DualSlope` when the centre of the travel should keep both dry and wet at full.
  - `MixLaw::Internal` for an oversampled effect that blends itself.
- `addSoloParameter()` if the effect adds something worth hearing on its own.

Override these as the effect needs:

- `latencyFrames()` if the effect delays its output.
- `factoryPresets()` and the preset hooks if it ships presets. A larger preset table goes in
  `<name>_presets.hpp/.cpp`.
- `setBpm()` if it syncs to tempo.

`src/domain/effects/gain.hpp/.cpp` is the smallest complete example.

New DSP building blocks go in `src/domain/dsp/` and are reused across effects and devices.

### 2. Build files

Add the `.hpp` to `HEADER_FILES` and the `.cpp` to `SOURCE_FILES` in `src/domain/CMakeLists.txt`,
in alphabetical order. That includes new `dsp/` sources. `src/domain/dsp/CMakeLists.txt` builds a
library that nothing links, so it doesn't need them.

### 3. Constants: `src/common/constants.hpp/.cpp`

- Add the type string to `Constants::RackEffectType`, for example `QString reverb224()`.
- Add any XML keys the effect needs to `Constants::NahdXml` that don't exist yet.

### 4. Factory: `src/domain/effects/effect_factory.cpp`

Register the effect in `EffectFactory::init()` by its type id:

```cpp
registerEffect(MyEffect::typeIdString(), []() { return std::make_shared<MyEffect>(); });
```

Older effects are also registered by their type string and under a snake_case
`registerLegacyEffect()` alias, because old projects saved those. A new effect has no such
projects, so the type id is enough.

### 5. Controller: `src/view/controllers/effect_rack_controller.hpp/.cpp`

- A `Q_PROPERTY(QString <name>Type READ <name>Type CONSTANT)` returning the
  `RackEffectType` string.
- One `Q_INVOKABLE QString <name><Param>Key() const` per parameter, returning its XML key. The
  dialog reads and writes values through `parameterValue(effectIndex, key)` and
  `setParameterValue(effectIndex, key, value)`.
- An `addEffect("<Display Name>", MyEffect::typeIdString(), <category>)` line in
  `availableEffects()`.
  - This list feeds the Effect Gallery.
  - The categories are `dynamics`, `eqAndFilter`, `distortion`, `modulation`, `space`, `stereo`,
    `utility` and `meters`.
  - Effect names are not translated.
- A branch in `effectParametersSummary()`, which gives the one-line summary the rack shows for the
  slot.

### 6. Dialog: `src/view/qml/Dialogs/<Name>Dialog.qml`

- Base it on `EffectDialog`, with an `effectIndex` property that the launcher sets.
- Size it in its own file as a fraction of the window:
  `width: parent ? parent.width * Constants.effectDialogScale : <design width>`.
- Put the content in a `ScrollView` (see [Dialog sizing](#dialog-sizing)).
- Bind each control's value to `effectRackController.revision`, so that it refreshes after
  automation, presets and undo. For example:

  ```qml
  Knob {
      label: qsTr("Gain")
      value: {
          effectRackController.revision;
          return effectRackController.parameterValue(root.effectIndex, effectRackController.gainGainKey());
      }
      onMoved: v => effectRackController.setParameterValue(root.effectIndex, effectRackController.gainGainKey(), v)
  }
  ```

Then wire it up:

- Add the file to `QML_SOURCE_FILES` in `src/CMakeLists.txt` (alphabetical).
- Instantiate it in `src/view/qml/Main.qml` with only `id` and `anchors.centerIn: parent`.
- Add a branch to `src/view/qml/Components/EffectDialogLauncher.qml`. It is the one place an
  effect type is mapped to its dialog:

  ```qml
  } else if (effectType === effectRackController.myEffectType) {
      myEffectDialog.effectIndex = index;
      myEffectDialog.open();
  ```

### 7. Tests

- `src/unit_tests/<name>_test/` with its own `CMakeLists.txt`, copied from a neighbour such as
  `reverb_224_test`. Add it to `src/unit_tests/CMakeLists.txt` with `add_subdirectory`. Test the
  DSP, not just that it runs: levels, frequency response, decay times, bypass at the defaults.
- A round-trip case in `xml_serialization_test`: put the effect in a project, save it, load it
  and compare every parameter.
- A case in `effect_rack_controller_test` that the gallery offers the effect, and that `setEffect`
  creates it.

### 8. Documentation, translations and changelog

See [Finishing a change](#finishing-a-change).

---

## Adding an internal device

A device takes MIDI and renders audio, or modulation, into a slot of the Virtual Device Rack.
`PianoSynthDevice` is the smallest complete device, and `Kick909Device` is a recent complete one
with presets, a waveform preview and MIDI CCs.

### 1. The device class: `src/domain/devices/<name>_device.hpp/.cpp`

Inherit from `Device` and implement:

| Method | Purpose |
|---|---|
| `std::string name() const` | The instance name it was constructed with. |
| `std::string category() const` | `Constants::NahdXml::xmlValueSynths()` or the like. |
| `std::string typeName() const` | `Constants::<name>DeviceName()`, the internal type name saved in projects. |
| `static std::string typeIdString()` / `typeId()` | A fresh UUID that never changes. |
| `processDeviceMidiNoteOn/Off(note, velocity)` | Notes, already shifted by the device's octave. |
| `processDeviceMidiCc(controller, value, channel)` | The device's own CCs (see below). |
| `processMidiAllNotesOff()` | Releases every voice. |
| `processAudio(AudioContext &)` | Renders and **adds** into `context.buffer` (interleaved stereo). |
| `bool hasActiveAudio() const` | False when silent, so the engine can skip the device. |
| `reset()` | Parameters back to defaults, then the audio state. |
| `resetDeviceAudio()` | Clears only the device's own audio state. `Device::resetAudio()` resets the insert rack first and then calls this. |
| `serializeToXml()` / `deserializeFromXml()` | Usually: attributes, MIDI mappings, insert effects, then the parameters. Copy them from a neighbour; the device-level test round-trips every device's mappings. |
| `syncParameters()` | Reads the parameters into the members the DSP runs on. |

Override these where they apply:

- `supportsOctave()`
- `modulationPriority()`: the parameters the LFO and XY Pad pick first.
- `deviceMidiCcControllers()`
- `advanceIdle()`: called instead of `processAudio` while the device is silent. A free-running
  LFO and its LED have to keep moving there.
- `processMidiProgramChange()` for presets.

**MIDI CCs.**
- `Device::processMidiCc()` is not virtual. It handles the CCs every device has, such as
  Expression, and passes the rest to `processDeviceMidiCc()`.
- Handle `ResetAllControllers` (clear automation), `ChannelVolumeMSB` (Fader) and `PanMSB`
  there, plus the device's own controllers.
- List them in `deviceMidiCcControllers()` so the automation editor can offer them.
- A CC moves a parameter with `setAutomationValue()`, not `setValue()`. Automation never
  overwrites what the user set.

**Threading.** `processAudio()` runs on the audio thread. Take the device's `mutex()` where the
state is shared with the UI. Do not allocate or emit signals in the audio path.

**Output level.** Devices are expected to peak at roughly -5 dBFS with their Init patch.
Remember that the centre pan law costs 3 dB.

**Presets.** Put presets in `<name>_presets.hpp/.cpp`, written in panel units (Hz, ms, dB)
through small helpers rather than raw 0..1 values. The device applies one by resetting every
parameter to its default first, so that a preset is the whole panel. See
`Kick909Device::applyPreset()`.

Heavy DSP goes in `src/domain/dsp/` (for example `dsp/drum/kick_909_engine.cpp`), so that the
device class stays a thin layer of parameters, voices and MIDI.

### 2. Build files

Add every new domain source (device, presets, DSP) to `HEADER_FILES` and `SOURCE_FILES` in
`src/domain/CMakeLists.txt`, alphabetically. Application sources, such as a waveform preview
service, go in `src/application/CMakeLists.txt`.

### 3. Constants: `src/common/constants.hpp/.cpp`

- Add `QString <name>DeviceName()`, the type name saved in projects, such as
  `"Noteahead Kick909"`.
- Add any new `Constants::NahdXml` keys. Reuse existing ones where they fit.

### 4. Factory: `src/domain/devices/device_registration.cpp`

```cpp
registerDevice(MyDevice::typeIdString(), [](const std::string & name) {
    return std::make_shared<MyDevice>(name);
});
```

### 5. Controller: `src/view/controllers/<name>_controller.hpp/.cpp`

- Inherit from `DeviceController` and implement:
  - `device()`
  - `setDevice(DeviceS)`: it casts to your type, then connects to the device's signals.
  - `requestSettings()`
  - `factoryPresetNames()` and `loadFactoryPreset()` if the device ships presets.
- One `Q_PROPERTY` per parameter:
  - `int` for continuous values, scaled by `Constants::uiInternalScaling()`, so 0..1 becomes
    0..1000.
  - `bool` for switches.
- Notify changes when the device emits `dataChanged` or `parametersChanged`, so the dialog
  follows presets, automation and undo.
- Add both files to `src/view/CMakeLists.txt`.

### 6. Device rack: `src/view/controllers/device_rack_controller.hpp/.cpp`

- Add a `void <name>DialogRequested();` signal.
- In `openDevice()`, add a branch that emits it:

  ```cpp
  } else if (typeId == MyDevice::typeIdString()) {
      emit myDeviceDialogRequested();
  ```

- Add an `addDevice("<Display Name>", QString::fromStdString(MyDevice::typeIdString()), <category>)`
  line in `availableDevices()`.
  - This feeds the Device Gallery.
  - The categories are `synths`, `keys`, `drums`, `samplers`, `mixers` and `modulators`.
  - Device names are not translated.

### 7. Application: `src/application/application.hpp/.cpp`

- Add a `std::shared_ptr<MyController> m_myController` member, constructed with a default device,
  as `m_kick909Controller` is.
- Add it to the controller vector passed to `DeviceRackController`. The rack hands the opened
  device to the controller through this vector.
- `qmlRegisterType<MyController>("Noteahead", majorVersion, minorVersion, "MyController");`
- `m_engine->rootContext()->setContextProperty("myController", m_myController.get());`

### 8. Dialog: `src/view/qml/Dialogs/<Name>Dialog.qml` and its sections

- Split a large panel into section files: `<Name>Dialog_Global.qml`, `<Name>Dialog_Presets.qml`
  and so on.
- Take the title from `myController.displayName`.
- Follow [Dialog sizing](#dialog-sizing).
- Most devices give the user the same Global controls:
  - Gain, Fader and Pan
  - LPF and HPF Cutoff (`FilterKnob`)
  - Velocity Sensitivity
  - Octave (`OctaveKnob`), where the device supports it
- Add the `VirtualKeyboard` component to play the device from the dialog.
- Add every file to `QML_SOURCE_FILES` in `src/CMakeLists.txt` (alphabetical).
- In `Main.qml`:
  - Instantiate the dialog (`id` and `anchors.centerIn: parent` only).
  - Connect the request in the same `Component.onCompleted` block as the other devices:

    ```qml
    deviceRackController.myDeviceDialogRequested.connect(myDeviceDialog.open);
    ```

### 9. Tests

- `src/unit_tests/<name>_test/` with its own `CMakeLists.txt`, plus `add_subdirectory` in
  `src/unit_tests/CMakeLists.txt`. Test:
  - that a note sounds and then goes silent
  - pitch, where the device is pitched
  - velocity
  - that `resetAudio()` renders identically again
  - that every preset loads, and that the preset names are unique
  - the device's MIDI CCs
- A project-level round-trip case in `xml_serialization_test`. It covers the factory
  registration too.
- `device_rack_controller_test` asserts the number of entries in `availableDevices()` and the
  position of named ones, so update it.

### 10. Documentation, translations and changelog

See below.

---

## Finishing a change

### Dialog sizing

The smallest window is 1024x768 (`Constants.minWindowWidth/minWindowHeight`), and every dialog
has to fit it:

- Size a dialog in its own file as a fraction of the window, such as
  `width: parent ? parent.width * Constants.largeDialogScale : <design width>`. Never set the size
  again in `Main.qml`.
- Put anything that can grow taller than the dialog in a `ScrollView` with:
  - `clip: true`
  - `ScrollBar.vertical.policy: ScrollBar.AsNeeded`
  - `ScrollBar.horizontal.policy: ScrollBar.AlwaysOff`

  Size its content with `width: <view>.availableWidth`.
- A child that has to keep its size inside a layout needs a `Layout.minimumHeight`. Otherwise a
  short dialog squeezes it. `VirtualKeyboard` declares its own.

Check the result at 1024x768 and at 1920x1080. The app can take screenshots of every dialog
headlessly:

```sh
QT_QPA_PLATFORM=offscreen ./build/noteahead --window-size 1024x768 --screenshots <dir>
```

### Translations

- Wrap every user-facing string in `qsTr()` in QML or `tr()` in C++.
  - Never put `qsTr()` inside a template literal: Qt 6.4's parser does not extract from those.
    Use `+` concatenation.
  - A `model: { ... }` block containing `qsTr()` has to read `languageService.activeLanguage` to
    retranslate.
- Do not translate:
  - device and effect names
  - unit strings (`"0 ms"`, `"12 dB/oct"`)
  - pan sides (`" L"`, `" R"`)
  - established audio jargon
- Regenerate the sources deliberately, never as part of a normal build:

  ```sh
  ninja -C build update_translations
  ```

  This runs Qt 6's `lupdate` with `-locations none -no-ui-lines -no-obsolete`. Location comments
  make every unrelated edit conflict, so never generate with them.
- Fill in the new `<message>` entries in each `src/translations/noteahead_<locale>.ts`. Before
  committing, check that the number of `type="unfinished"` entries matches what it was before,
  unless you meant to leave some.
- Renaming a `.qml` file changes its translation context. `lupdate` then marks its finished
  strings unfinished, so restore them.

### Unit tests

- One test per subdirectory of `src/unit_tests/`, each with its own `CMakeLists.txt`.
- Test headers include `<QObject>` only. Test sources include `<QTest>`, not `<QtTest>`, plus
  specific headers such as `<QSignalSpy>`. Do not include `.moc` files.
- Use `QTEST_GUILESS_MAIN`. A test that needs a `QGuiApplication` (fonts, QPainter, windows) uses
  `QTEST_MAIN` and sets the platform in its `CMakeLists.txt`:

  ```cmake
  set_tests_properties(${NAME} PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
  ```

- Name test functions `test_<subject>_<variant>_<condition>`, for example
  `test_noiseFx_outputFilter_shouldCutTheLevel`.

### Documentation and changelog

- `CHANGELOG`: one concise entry in the unreleased section, under *New features*, *Bug fixes* or
  *Other*.
- `README.md`: add a new device to the instrument list and its count. Add a new effect wherever
  the effects are listed.
- `src/view/qml/Manual.html`: add a section for the new device or effect, and its name to the
  gallery lists.

### Commits

- Imperative mood, first line under 50 characters: "Add Kick 909", "Fix ...".
- If there is a body, write it as `-` bullets with a blank line between each, and keep it short.
  The detail belongs in the code comments and the review, not the log.
- Do not commit build output, local tool directories, recordings or reference samples.

### Checklist

Effect:

- [ ] `domain/effects/<name>.hpp/.cpp`, plus DSP in `domain/dsp/` and presets as needed
- [ ] `src/domain/CMakeLists.txt`
- [ ] `RackEffectType` and any new XML keys in `constants.hpp/.cpp`
- [ ] `EffectFactory::init()`
- [ ] `EffectRackController`: type property, key methods, `availableEffects()`, summary
- [ ] Dialog, `src/CMakeLists.txt`, `Main.qml`, `EffectDialogLauncher.qml`
- [ ] Effect test, `xml_serialization_test`, `effect_rack_controller_test`
- [ ] Translations, `CHANGELOG`, `README.md`, `Manual.html`

Device:

- [ ] `domain/devices/<name>_device.hpp/.cpp`, plus presets and DSP as needed
- [ ] `src/domain/CMakeLists.txt` (and `src/application/CMakeLists.txt` for services)
- [ ] `<name>DeviceName()` and any new XML keys in `constants.hpp/.cpp`
- [ ] `device_registration.cpp`
- [ ] Controller, `src/view/CMakeLists.txt`
- [ ] `DeviceRackController`: signal, `openDevice()`, `availableDevices()`
- [ ] `Application`: member, controller vector, `qmlRegisterType`, context property
- [ ] Dialog and sections, `src/CMakeLists.txt`, `Main.qml` instance and connection
- [ ] Device test, `xml_serialization_test`, `device_rack_controller_test`
- [ ] Translations, `CHANGELOG`, `README.md`, `Manual.html`
