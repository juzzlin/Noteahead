# Noteahead design notes

## Project file format (NAHD)

A project is an XML file, `.nahd`. These rules keep it readable, diffable and the same on every
machine.

### Numbers

- **Integers wherever a value has a fixed range.**
  - A device or effect parameter is saved as its knob position scaled onto the integer range
    written beside it: `value="6430" min="0" max="10000"` is 64.3 % of the knob's travel. Loading
    reads `value`, `min` and `max`. `scale` is only a divisor some effects use to read the integer in
    their own units; loading ignores it.
  - Any other value normalized to 0…1 (or -1…1) is saved as an integer with **10000 for 1**,
    through `Utils::Xml::normalizedToXml()` and `readNormalizedAttribute()`. This covers send
    levels (`level="940"`), the Min/Max of a modulator's MIDI target (`min="8000"`) and the range
    of a device's MIDI mapping.
  - MIDI values are saved in MIDI's own integers: CC 0–127, pitch bend in whole percent.
- **Decimals only where a value has no grid of its own.** Parameter automation positions
  (`value0`/`value1`) and the nodes of a drawn automation curve are saved with 12 significant
  digits, so that a drawn curve comes back exactly as it was drawn rather than snapped onto a grid.
- **Never in the system's locale.** Numbers are written with `QString::number()` and read with
  `QString::toInt()`/`toDouble()`, which always use a decimal point. Never use `QLocale`,
  `std::stod`/`std::stof`, `printf`-style formatting or streams for anything that goes into a
  project: in a Finnish or German locale they write and read a decimal comma.
  `xml_serialization_test` runs a round trip in the Finnish locale to hold this.

### Compatibility

- A project must keep loading in every later version. A new attribute defaults to the old
  behaviour, and is omitted when it holds the default.
- When a value's format changes, write it under a new attribute name and keep reading the old one.
  Reusing the name would make an old value ambiguous: `value="1"` could be a whole send level in an
  old file or 1/10000 of one in a new file. Example: sends are written as `level` and an older
  `value` is still read.
- Parameter keys are generic, and renamed keys are registered as legacy names; see `CLAUDE.md`.

### Elements and attributes

Element and attribute names are defined once, in `Constants::NahdXml`. A name in a saved file is
frozen, misspellings included (`ColumMuted`). Attributes marked *optional* are left out when they
hold their default, and an element that holds nothing is usually left out too.

```text
Project                    fileFormatVersion applicationName applicationVersion createdDate
├─ Song                    beatsPerMinute linesPerBeat timeSpent(optional, seconds)
│  ├─ Metadata
│  │  ├─ Tags / Tag        name value
│  │  ├─ ExportTags / Tag  name value
│  │  ├─ Notes             text content (optional)
│  │  └─ RenderSettings    format sampleRate bitDepth oversampleFactor fastRender
│  │                       normalizeEnabled normalizeLevelTenthsDb trimEnabled trimMinutes
│  │                       trimSeconds fadeOutEnabled fadeOutSeconds fadeOutTenths
│  │                       silenceEnabled silenceSeconds silenceTenths analyzeEnabled
│  ├─ SongSettings         autoNoteOffOffset autoNoteOffSyncEnabled autoNoteOffSyncDenominator
│  ├─ PlayOrder / Position index pattern skipped(optional)
│  ├─ Mixer
│  │  ├─ ColumMuted, ColumSoloed            track column
│  │  ├─ TrackMuted, TrackSoloed            index
│  │  ├─ ColumnVelocityScale                track column value
│  │  └─ TrackVelocityScale                 index value
│  ├─ Automation
│  │  ├─ MidiCcAutomation  controller eventsPerBeat lineOffset comment enabled
│  │  ├─ PitchBendAutomation               eventsPerBeat lineOffset comment enabled
│  │  └─ ParameterAutomation               mode(optional: relative) eventsPerBeat lineOffset
│  │                                       comment enabled
│  │     each with:
│  │     ├─ Location       pattern track column
│  │     ├─ Target         owner ownerId parameter (ParameterAutomation only)
│  │     ├─ Interpolation  line0 line1 value0 value1 curve(optional)
│  │     │  └─ Node        line value (only for a Drawn curve)
│  │     └─ Modulation     type(SineWave, Random) cycles amplitude offset inverted (optional)
│  ├─ Devices
│  │  ├─ Device            slot name typeName category typeId uuid octave(optional)
│  │  │  ├─ MidiMappings / MidiMapping     kind(optional: cc) controller target min max
│  │  │  ├─ InsertEffects  enabled
│  │  │  │  └─ Effect      typeId type id enabled, holding Parameters
│  │  │  ├─ Parameters / Parameter         see below
│  │  │  └─ device-specific children:
│  │  │     ├─ Voices / Voice index, holding InsertEffects      (Drum Synth, drum pads)
│  │  │     ├─ Pads / Pad index note, holding InsertEffects     (Sampler)
│  │  │     ├─ Samples / Sample note path                       (Sampler)
│  │  │     ├─ Members / Member slot                            (Sub Mixer)
│  │  │     ├─ Lfos / Lfo index owner ownerId parameter min max (LFO)
│  │  │     ├─ Pads / Pad index axis(0 = x, 1 = y) owner ownerId parameter min max (XY Pad)
│  │  │     └─ phrase attribute on Device                       (Speech)
│  │  └─ MasterEffects
│  │     ├─ InsertEffects  enabled, holding Effect
│  │     └─ SendEffects    enabled, holding Effect
│  │        ├─ Send        deviceSlot subIndex(optional) effectSlot level
│  │        └─ SendChains / SendChain index, holding InsertEffects
│  ├─ SideChain / SideChainSettings        track enabled sourceTrack sourceColumn lookahead release
│  │  └─ Target            enabled controller targetValue releaseValue
│  ├─ AudioRecorder        latestRecordingFilePath latestRecordingStartTick latestRecordingEndTick
│  └─ Patterns / Pattern   index name lineCount trackCount
│     └─ Tracks / Track    index name lineCount columnCount columnIndices(optional)
│        ├─ Instrument     portName channel group
│        │  └─ InstrumentSettings          see below
│        └─ Columns / Column               index name lineCount
│           ├─ Lines / Line                index (only lines that hold something)
│           │  ├─ NoteData  type(noteOn, noteOff, none) note velocity delay pan
│           │  └─ LineEvent, holding InstrumentSettings
│           └─ ColumnSettings              delay transpose midiDelay* chordNote1..3Offset/Velocity/Delay
│                                          arpeggiatorEnabled arpeggiatorPattern
│                                          arpeggiatorEventsPerBeat
├─ PerformancePanel / Element              type sourceId
└─ Data                    path, base64 content of an embedded sample
```

- **Parameter**: `name valueType value`, where `valueType` is `float`, `int` or `bool`. A `float`
  also has `min max default scale`; see *Numbers* above.
- **InstrumentSettings**: `patch bankEnabled bankLsb bankMsb bankByteOrderSwapped transpose
  drumTrack sendMidiClock sendTransport delay velocityJitter velocityKeyTrack
  velocityKeyTrackOffset`, the auto note-off attributes of `SongSettings`, and `MidiCcSetting`
  children (`enabled controller value`). Optional settings are left out when unset.
- **Targets** (`Target`, `Lfo`, XY `Pad`): `owner` is `effect`, `device` or `midi`. For `midi`,
  `ownerId` is the port and `parameter` is `channel:controller` or `channel:pb`; `min` and `max`
  appear only on a MIDI target whose range is not the whole of it.
- **Curves**: `curve` is `Linear` (left out), `Exponential`, `Logarithmic`, `EaseIn`, `EaseOut`,
  `EaseInOut` or `Drawn`.
- **Unknown devices**: a `Device` this version cannot create is written back as it was read, so
  saving never loses what a newer version made.

Other files use the same elements under another root:

- **Exported device or effect settings**: `Settings` (with the `Project` attributes), holding one
  `Device`, `Effect` or `InsertEffects` and any `Data` a Sampler embeds.
- **User presets**: `Preset fileFormatVersion applicationName applicationVersion createdDate typeId
  name`, holding `Parameters`.

## Effects and devices as plugins

Every effect and device lives in a folder of its own in `src/plugins/`. The steps for adding one
are in `CONTRIBUTING.md`; this is why it is built the way it is.

- **A folder, not a list.** Adding an effect used to mean editing a dozen lists across the tree:
  the factory, the gallery, the controller, `Main.qml`, the launcher and the build files. Now
  nothing outside the folder names it, except one `add_subdirectory()`, so the shared classes no
  longer grow with every plugin.
- **Layers still hold.** A folder mixes layers, but each file is built into the library of its own
  layer, so domain code in a folder can no more reach the view than the rest of the domain can.
- **Registration is generated, not static.** The libraries are static, so a self-registering
  global object would be dropped by the linker. CMake instead writes one function per kind and
  layer that calls every plugin's `register...()` function by name.
  - The effects and devices get separate files, so that a binary that uses only effects does not
    link every device.
  - The domain half fills `EffectFactory` and `DeviceFactory`. The view half fills
    `PluginCatalog`: gallery entries, dialog files, summaries, QML types and, for a device, its
    dialog model.
- **Dialogs are created on first open.** `PluginDialogHost` loads a plugin's dialog from the file
  the catalog names and keeps it. Plugin dialogs keep their old resource path, `view/qml/Dialogs/`,
  so their relative imports and translation contexts did not change when they moved.
- **Dialog models hold what is not a parameter.** Each effect's response curves, meter readings
  and multi-parameter actions are in its own `EffectDialogModel`, which looks its effect up through
  the rack controller every time. A slot can change under an open dialog, so a model never keeps
  the effect it was opened for. Device dialog models are one per device type, and the rack hands
  them the device being opened.
- **Rack controllers are split by subject.** Parameter keys come from the `NahdXml` singleton. The
  mixer, the meters, the MIDI mappings and the sends each have a controller of their own, so
  `EffectRackController` and `DeviceRackController` keep only the rack.
