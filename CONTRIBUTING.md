# Contributing to Noteahead

Noteahead is a MIDI tracker and sequencer for Linux, written in C++20 and Qt 6/QML. This guide
covers the workflow and conventions, and then walks through the two most common kinds of
contribution in detail: a new **rack effect** and a new **internal device** (an instrument, a
mixer or a modulator in the Virtual Device Rack).

Every effect and device lives in a folder of its own, so the quickest way to learn the shape of
one is to read a folder end to end:

- An effect: `src/plugins/effects/gain/`, the smallest; `src/plugins/effects/eq_8_band_parametric/`
  for one with a dialog model.
- A device: `src/plugins/devices/kick_808/`, the smallest; `src/plugins/devices/kick_909/` for one
  with presets, a waveform preview and MIDI CCs.

Every file in such a folder is described below, with what goes into it.

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
| `src/domain/` | Pure logic: the tracker model, the `Device` and `Effect` bases, DSP. No UI. |
| `src/application/` | Services that orchestrate the domain, QML list models, undo/redo commands. |
| `src/infra/` | Adapters: XML, MIDI, audio backends. |
| `src/view/` | Thin `QObject` controllers for QML, and all QML. No logic. |
| `src/common/` | Constants, including every XML key string, and small utilities. |
| `src/plugins/` | One folder per effect or device, holding all of it across the layers above. |

A device or an effect lives in its folder in `src/plugins/effects/` or `src/plugins/devices/`.
Each file there is still built into the library of its own layer, so the layer rules hold inside
a folder too: the effect or device class is domain code, its dialog model is view code, and so on.
QML reaches a device only through its dialog model or the controllers in `view/controllers/`, and
dialogs are opened only through the `UiService` QML singleton, a controller signal connected in
`Main.qml`, or the `PluginDialogHost` that creates plugin dialogs. A controller never opens a
dialog itself.

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
| Dialog models | `PascalCase` + `DialogModel` | `Kick909DialogModel`, `Eq8BandParametricDialogModel` |
| Context properties for controllers and device dialog models | `camelCase` | `effectRackController`, `kick909Controller` |
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

### 1. The effect class: `src/plugins/effects/<name>/<name>.hpp/.cpp`

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

`src/plugins/effects/gain/` is the smallest complete example.

New DSP building blocks go in `src/domain/dsp/` and are reused across effects and devices. List
them in `src/domain/CMakeLists.txt` (alphabetically); `src/domain/dsp/CMakeLists.txt` builds a
library that nothing links.

### 2. The folder's build file: `src/plugins/effects/<name>/CMakeLists.txt`

One call lists the folder's files by the layer each is built into:

```cmake
noteahead_add_effect(
    DOMAIN_SOURCES
        my_delay.cpp
        my_delay.hpp
        my_delay_registration.cpp
    VIEW_SOURCES
        my_delay_view_registration.cpp
    QML_FILES
        MyDelayDialog.qml
)
```

Add `add_subdirectory(effects/<name>)` to `src/plugins/CMakeLists.txt`, alphabetically. A
`<name>_presets.hpp/.cpp` or a dialog model joins the list of its layer.

### 3. Constants: `src/common/constants.hpp/.cpp`

- Add the type string to `Constants::RackEffectType`, for example `QString reverb224()`.
- Add any XML keys the effect needs to `Constants::NahdXml` that don't exist yet, and expose a new
  key to QML by adding it to `NahdXmlKeys` in `src/view/controllers/nahd_xml_keys.hpp/.cpp`.

### 4. Registration

Two small files register the effect. The calls to them are generated from their file names, so
no list anywhere else names the effect.

`<name>_registration.cpp` adds it to the factory:

```cpp
void registerMyDelayEffect()
{
    EffectFactory::registerEffect(MyDelay::typeIdString(), [] { return std::make_shared<MyDelay>(); });
}
```

Older effects also register their type string and a snake_case `registerLegacyEffect()` alias,
because old projects saved those. A new effect has no such projects, so the type id is enough.

`<name>_view_registration.cpp` adds its entry to the `PluginCatalog`:

```cpp
void registerMyDelayEffectView()
{
    PluginCatalog::addEffect({
      .name = "My Delay",
      .typeId = MyDelay::typeIdString(),
      .type = Constants::RackEffectType::myDelay(),
      .category = PluginCatalog::EffectCategory::Space,
      .dialogFile = "MyDelayDialog.qml",
      .summary = [](const std::shared_ptr<Effect> & effect, const PluginCatalog::SummaryContext &) -> QString {
          // The one-line summary the rack shows for the slot
      },
    });
}
```

- The entry feeds the Effect Gallery, which lists effects alphabetically. Effect names are not
  translated.
- The categories are `Dynamics`, `EqAndFilter`, `Distortion`, `Modulation`, `Space`, `Stereo`,
  `Utility` and `Meters`.
- Translate any words in the summary with `EffectSummary::tr()`.
- Nothing is added to `EffectRackController`.

### 5. Dialog: `<Name>Dialog.qml` in the effect's folder

- Base it on `EffectDialog`, with an `effectIndex` property that the launcher sets.
- Size it in its own file as a fraction of the window:
  `width: parent ? parent.width * Constants.effectDialogScale : <design width>`. Nothing else sizes
  it.
- Put the content in a `ScrollView` (see [Dialog sizing](#dialog-sizing)).
- Read the keys from the `NahdXml` singleton, and bind each control's value to
  `effectRackController.revision`, so that it refreshes after automation, presets and undo. For
  example:

  ```qml
  Knob {
      label: qsTr("Gain")
      value: {
          effectRackController.revision;
          return effectRackController.parameterValue(root.effectIndex, NahdXml.gain);
      }
      onMoved: v => effectRackController.setParameterValue(root.effectIndex, NahdXml.gain, v)
  }
  ```

- The top of a discrete parameter's range comes from
  `effectRackController.parameterMaximum(root.effectIndex, key)`.

The dialog is not instantiated in `Main.qml`, and `EffectDialogLauncher.qml` has no branch for it:
the launcher asks the catalog for the dialog file, and `PluginDialogHost` creates the dialog the
first time it opens.

### 6. Dialog model, when the dialog needs more than parameters

A response curve, a meter reading or an action that sets several parameters at once goes in a
`<Name>DialogModel` in the effect's folder, listed under `VIEW_SOURCES`:

- Derive it from `EffectDialogModel` and read the effect with `effect()`, which is null when the
  slot holds something else. Set several parameters with one redraw through
  `setParameterValues()`.
- Register it from the catalog entry:
  `.registerQmlTypes = [] { qmlRegisterType<MyDelayDialogModel>("Noteahead", 1, 0, "MyDelayDialogModel"); }`.
- Create it in the dialog for its own slot:

  ```qml
  MyDelayDialogModel {
      id: dialogModel
      rack: effectRackController
      effectIndex: root.effectIndex
  }
  ```

- A binding that reads the model also reads `dialogModel.effectIndex`, so that it follows the
  dialog to another slot.
- An inline `component` cannot see the dialog's ids. Pass the model in through a property, and
  give that property another name than the id, such as `effectModel: dialogModel`; a property
  named `dialogModel` would bind to itself.

`src/plugins/effects/eq_8_band_parametric/` is the example.

### 7. Tests

- The effect's tests go in `unit_tests/<name>_test/` in its folder, with their own
  `CMakeLists.txt` copied from a neighbour such as `reverb_224`'s. Every folder in `unit_tests/`
  that has one is added. Test the DSP, not just that it runs: levels, frequency response, decay
  times, bypass at the defaults. A dialog model gets a test of its own beside it, such as
  `eq_8_band_parametric_dialog_model_test`.
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

### 1. The device class: `src/plugins/devices/<name>/<name>_device.hpp/.cpp`

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

### 2. The folder's build file: `src/plugins/devices/<name>/CMakeLists.txt`

One call lists the folder's files by the layer each is built into:

```cmake
noteahead_add_device(
    DOMAIN_SOURCES
        my_synth_device.cpp
        my_synth_device.hpp
        my_synth_presets.cpp
        my_synth_presets.hpp
        my_synth_registration.cpp
    APPLICATION_SOURCES
        my_synth_preview.cpp
        my_synth_preview.hpp
    VIEW_SOURCES
        my_synth_dialog_model.cpp
        my_synth_dialog_model.hpp
        my_synth_view_registration.cpp
    QML_FILES
        MySynthDialog.qml
        MySynthDialog_Global.qml
)
```

Add `add_subdirectory(devices/<name>)` to `src/plugins/CMakeLists.txt`, alphabetically. Leave out
the layers the device has nothing in. Shared DSP goes in `src/domain/dsp/` and is listed in
`src/domain/CMakeLists.txt` instead.

### 3. Constants: `src/common/constants.hpp/.cpp`

- Add `QString <name>DeviceName()`, the type name saved in projects, such as
  `"Noteahead Kick909"`.
- Add any new `Constants::NahdXml` keys. Reuse existing ones where they fit.

### 4. Dialog model: `<name>_dialog_model.hpp/.cpp` in the device's folder

- Inherit from `DeviceDialogModel` and implement:
  - `device()`
  - `setDevice(DeviceS)`: it casts to your type, then connects to the device's signals.
  - `requestSettings()`
  - `factoryPresetNames()` and `loadFactoryPreset()` if the device ships presets.
  - `retranslate()` if it builds lists with `tr()`, so that a language change reaches them.
- One `Q_PROPERTY` per parameter:
  - `int` for continuous values, scaled by `Constants::uiInternalScaling()`, so 0..1 becomes
    0..1000.
  - `bool` for switches.
- Notify changes when the device emits `dataChanged` or `parametersChanged`, so the dialog
  follows presets, automation and undo.

There is one dialog model per device type. The rack hands it whichever device of that type is
being opened.

### 5. Registration

Two small files register the device. The calls to them are generated from their file names, so
nothing in `DeviceFactory`, `DeviceRackController`, `Application` or `Main.qml` names the device.

`<name>_registration.cpp` adds it to the factory:

```cpp
void registerMySynthDevice()
{
    DeviceFactory::registerDevice(MySynthDevice::typeIdString(), [](const std::string & name) {
        return std::make_shared<MySynthDevice>(name);
    });
}
```

`<name>_view_registration.cpp` adds its entry to the `PluginCatalog`:

```cpp
void registerMySynthDeviceView()
{
    PluginCatalog::addDevice({
      .name = "My Synth",
      .typeId = MySynthDevice::typeIdString(),
      .category = PluginCatalog::DeviceCategory::Synths,
      .dialogFile = "MySynthDialog.qml",
      .controllerProperty = "mySynthController",
      .createDialogModel = [](const PluginCatalog::DeviceServiceS &) {
          return std::make_shared<MySynthDialogModel>(std::make_shared<MySynthDevice>("Default MySynth"));
      },
      .registerQmlTypes = [] { qmlRegisterType<MySynthDialogModel>("Noteahead", 1, 0, "MySynthDialogModel"); },
    });
}
```

- The entry feeds the Device Gallery, which lists devices alphabetically. Device names are not
  translated.
- The categories are `Synths`, `Keys`, `StringsAndVoice`, `Drums`, `Samplers`, `Mixers` and
  `Modulators`.
- `controllerProperty` is the name the dialog reaches its model by.
- `createDialogModel` is given the device service, for a model that needs it.
- A model that needs a service that only exists later, such as the `AudioService`, gets it from
  `connectServices`. `src/plugins/devices/sampler/` is the example.

### 6. Dialog: `<Name>Dialog.qml` and its sections, in the device's folder

- Split a large panel into section files: `<Name>Dialog_Global.qml`, `<Name>Dialog_Presets.qml`
  and so on, all listed under `QML_FILES`.
- Take the title from `mySynthController.displayName`.
- Follow [Dialog sizing](#dialog-sizing).
- Most devices give the user the same Global controls:
  - Gain, Fader and Pan
  - LPF and HPF Cutoff (`FilterKnob`)
  - Velocity Sensitivity
  - Octave (`OctaveKnob`), where the device supports it
- Add the `VirtualKeyboard` component to play the device from the dialog.

The dialog is not instantiated in `Main.qml`: `openDevice()` emits `pluginDeviceDialogRequested`
with the catalog's dialog file, and `PluginDialogHost` creates the dialog the first time it opens.

### 7. Tests

- The device's tests go in `unit_tests/<name>_test/` in its folder, with their own
  `CMakeLists.txt`; a dialog model gets `unit_tests/<name>_dialog_model_test/`. Every folder in
  `unit_tests/` that has a `CMakeLists.txt` is added. Test:
  - that a note sounds and then goes silent
  - pitch, where the device is pitched
  - velocity
  - that every preset loads, and that the preset names are unique
  - the device's MIDI CCs
- A project-level round-trip case in `xml_serialization_test`. It covers the factory
  registration too.
- `device_service_test` already checks every registered device: that its parameter defaults are
  where it starts, that its modulation priorities name its own parameters, and that
  `resetAudio()` renders identically again. `device_rack_controller_test` checks the gallery's
  list of names, so add the new one there.

### 8. Documentation, translations and changelog

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

- One test per subdirectory, each with its own `CMakeLists.txt`: in the plugin's `unit_tests/`
  for an effect's or device's own tests, in `src/unit_tests/` for everything else.
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

Effect, all in `src/plugins/effects/<name>/`:

- [ ] `<name>.hpp/.cpp`, plus DSP in `src/domain/dsp/` and presets as needed
- [ ] `CMakeLists.txt` with `noteahead_add_effect()`, and `add_subdirectory` in `src/plugins/CMakeLists.txt`
- [ ] `RackEffectType` and any new XML keys in `constants.hpp/.cpp`, new keys in `NahdXmlKeys`
- [ ] `<name>_registration.cpp` and `<name>_view_registration.cpp`, with the summary
- [ ] `<Name>Dialog.qml`, and a `<Name>DialogModel` if it needs more than parameters
- [ ] Tests in `unit_tests/`, `xml_serialization_test`, `effect_rack_controller_test`
- [ ] Translations, `CHANGELOG`, `README.md`, `Manual.html`

Device, all in `src/plugins/devices/<name>/`:

- [ ] `<name>_device.hpp/.cpp`, plus presets and DSP as needed
- [ ] `CMakeLists.txt` with `noteahead_add_device()`, and `add_subdirectory` in `src/plugins/CMakeLists.txt`
- [ ] `<name>DeviceName()` and any new XML keys in `constants.hpp/.cpp`
- [ ] `<name>_dialog_model.hpp/.cpp`
- [ ] `<name>_registration.cpp` and `<name>_view_registration.cpp`
- [ ] `<Name>Dialog.qml` and its sections
- [ ] Tests in `unit_tests/`, `xml_serialization_test`, `device_rack_controller_test`
- [ ] Translations, `CHANGELOG`, `README.md`, `Manual.html`
