# Claude Instructions for Noteahead

Noteahead is a MIDI tracker and sequencer for Linux, written in **Qt/QML** and **C++20**. Refer to [Agents.md](./Agents.md) for the foundational architecture, technology stack, and coding principles. This file records Claude-specific mandates and context built up over sessions.

## Build & Test

- **Build**: `cmake --build build`
- **Run tests**: `ctest --test-dir build`
- Always build and run tests after making changes. Fix all failures before reporting done.

## Architecture

See [Agents.md](./Agents.md) for the full layer description. Key points:

- `src/domain` — pure business logic; no UI dependencies.
- `src/application` — services and models bridging domain and view.
- `src/view` — QML only; no logic.
- `src/common/constants.hpp` — all XML key constants live here. Never hardcode key strings outside this file.

## XML Serialization

- Parameter XML keys must be **generic** (no device/effect prefix). E.g., `"lpfCutoff"`, not `"synthLpfCutoff"`.
- When renaming a key, register the old name(s) as legacy names in the `Parameter` constructor's `LegacyNameList` argument. This ensures old project files still load.
- `ParameterContainer` maps legacy names to current names at deserialization time.
- Every renamed key must have a corresponding legacy-name round-trip test in `xml_serialization_test`.

## Coding Standards

Follow [Agents.md](./Agents.md) strictly. Highlights:

- C++20; all code in the `noteahead` namespace.
- `const` everywhere applicable. All non-mutating methods must be `const`.
- Smart pointers only; no raw `new`/`delete`.
- `=` for local variable initialization; `{}` for class instantiation only.
- Post-increment (`i++`) for loop counters.
- No string literals duplicated — centralize in `constants.hpp`.
- All implementation in `.cpp`; headers contain declarations only.
- `#ifndef FILENAME_HPP` header guards, not `#pragma once`.

## Unit Tests

- Each test in its own subdirectory under `src/unit_tests/` with its own `CMakeLists.txt`.
- Test headers (`.hpp`): include `<QObject>` only — never `QtTest`.
- Test sources (`.cpp`): use `#include <QTest>` (not `<QtTest>`); add specific headers like `<QSignalSpy>` as needed.
- Do not manually include `.moc` files; rely on CMake's automatic MOC.
- Test function naming: `test_<subject>_<variant>_<condition>` (variant goes after subject, not merged into it).

## Git

- Imperative tense in commit messages ("Add", "Fix", "Update" — not "Added", "Fixed").
- First line under 50 characters.
