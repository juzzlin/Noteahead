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
    levels (`level="940"`) and the Min/Max of a modulator's MIDI target (`min="8000"`).
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
