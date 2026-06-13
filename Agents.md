# Noteahead: Agent Guide

Noteahead is a MIDI tracker and sequencer for Linux, written in **Qt/QML** and **C++20**. It follows modern software engineering practices, prioritizing performance, accuracy, and maintainability.

## 🏗 Architecture

The project follows a layered architecture to ensure separation of concerns:

- **`src/domain`**: The "Heart" of the application. Contains pure business logic and data structures.
    - **`devices/`**: Virtual instruments and effects (e.g., `SynthDevice`, `DelayEffect`).
    - **`dsp/`**: Digital Signal Processing components (e.g., `AdsrEnvelope`, `SvfFilter`).
    - **Core**: `Song`, `Track`, `Pattern`, `Arpeggiator`, etc.
    - It should remain as independent of the UI and infrastructure as possible.
- **`src/application`**: The "Brain". Orchestrates domain objects and provides services for the UI.
    - **`service/`**: High-level logic (e.g., `MidiService`, `EditorService`).
    - **`models/`**: `QAbstractListModel` and other QML-facing models.
    - **`command/`**: Implementation of the Command Pattern for Undo/Redo functionality.
- **`src/infra`**: The "Hands". Handles external systems:
    - **`midi/`**: RtMidi backend and MIDI file export/import.
    - **`audio/`**: RtAudio backend and audio recording.
    - **`settings.cpp/hpp`**: Persistent configuration management.
- **`src/view`**: The "Face". Pure QML-based UI, communicating with the application layer via models and services.
- **`src/common`**: Shared constants and utilities.
- **`src/contrib`**: External dependencies bundled with the source (`Argengine`, `SimpleLogger`).

## 🛠 Technology Stack

- **Language**: C++20 (Standard strictly required).
- **UI Framework**: Qt 6 (6.4+).
- **Build System**: CMake with Ninja.
- **Backend**:
  - **MIDI**: RtMidi.
  - **Audio**: RtAudio / JACK.
  - **File IO**: libsndfile, Qt XML.
- **Testing**: CTest + Qt Test.

## 📜 Coding Standards & Style

Adherence to these standards is mandatory for all contributions:

### General Principles
- **Clean Code**: Prioritize readability and self-documenting code.
- **Const-Correctness**: Embrace `const` everywhere. Methods that do not modify state MUST be marked `const`.
- **Memory Management**: Use smart pointers (`std::unique_ptr`, `std::shared_ptr`) and avoid raw `new`/`delete`.
- **RAII**: Resource acquisition is initialization.
- **No Copy-Paste**: Never duplicate string literals or logic. Centralize strings in `src/common/constants.hpp` or relevant classes.
- **Implementation Location**: All implementation MUST reside in `.cpp` files. Headers MUST only contain declarations.

### C++ Specifics
- **Initialization**: Use `=` for initializing local normal variables (e.g., `int i = 0;`). Use braced initializers `{}` only for class instantiations to avoid narrowing conversions and the "most vexing parse".
- **Namespace**: All code belongs in the `noteahead` namespace.
- **Formatting**: Strictly follow the project's `.clang-format` located in the root.
- **Header Guards**: Use `#ifndef FILENAME_HPP` style instead of `#pragma once`.
- **Standard Library**: Prefer `std::` containers and algorithms where appropriate. Use modern C++20 features like `std::format` for string formatting and `if` statements with initializers (e.g., `if (const auto it = map.find(key); it != map.end())`).
- **Post-increment**: Always use post-increment (`i++`) instead of pre-increment (`++i`) for loop counters.

### XML Serialization
- **Unified Keys**: XML keys MUST be standardized and prefix-less (e.g., `ampAttack` instead of `wavetableSynthAmpAttack`).
- **Legacy Support**: Maintain backward compatibility with older project files by using the legacy name mapping system in `ParameterContainer`.
- **Centralized Constants**: All XML key strings MUST be defined in `src/common/constants.hpp`.

### Qt & QML
- **Model/View**: Keep logic in C++ models and services; QML should only handle presentation.
- **Properties**: Use `Q_PROPERTY` with `READ`, `WRITE`, and `NOTIFY` signals correctly.
- **Signals/Slots**: Prefer the modern `connect` syntax (`&Sender::signal, receiver, &Receiver::slot`).

## 📝 Git & Commits

- **Imperative Tense**: ALWAYS use the imperative tense in commit messages ("Fix bug" not "Fixed bug", "Update docs" not "Updating docs").
- **Conciseness**: Keep the first line of the commit message brief (ideally under 50 characters).

## 🧪 Testing Strategy

- **Unit Tests**: Located in `src/unit_tests`. Every new feature or bug fix should include a test.
- **Execution**: Run tests using `ctest` from the build directory.
- **Framework**: Uses the Qt Test framework.
- **Project Structure**: Each test MUST be in its own subdirectory with its own `CMakeLists.txt`.
- **Lean Headers**: Test headers (`.hpp`) MUST NOT include `QtTest`. They MUST only include `<QObject>`.
- **Granular Includes**: In test source files (`.cpp`), NEVER use `#include <QtTest>`. Use the specific `#include <QTest>` header and other specific utilities (e.g., `<QSignalSpy>`) as needed.
- **MOC Handling**: DO NOT explicitly include `.moc` files (e.g., `#include "test.moc"`) in source files. Rely on CMake's automatic MOC handling by including the header in the `qt_add_executable` source list.

## 🚀 Build & Test

- **Build**: Use `cmake --build build` to build the project.
- **Run Tests**: Use `ctest --test-dir build` to run all tests.

## 🚀 Performance & Timing

- **Accuracy**: Noteahead renders events just before playback to ensure jitter-free, drift-free timing.
- **Thread Safety**: The player runs in a dedicated thread (`PlayerWorker`). Use safe synchronization primitives when interacting between the UI and player threads.

## 🤖 General LLM Coding Guidelines

Behavioral guidelines to reduce common LLM coding mistakes.

**Tradeoff:** These guidelines bias toward caution over speed. For trivial tasks, use judgment.

### 1. Think Before Coding

**Don't assume. Don't hide confusion. Surface tradeoffs.**

Before implementing:
- State your assumptions explicitly. If uncertain, ask.
- If multiple interpretations exist, present them - don't pick silently.
- If a simpler approach exists, say so. Push back when warranted.
- If something is unclear, stop. Name what's confusing. Ask.

### 2. Simplicity First

**Minimum code that solves the problem. Nothing speculative.**

- No features beyond what was asked.
- No abstractions for single-use code.
- No "flexibility" or "configurability" that wasn't requested.
- No error handling for impossible scenarios.
- If you write 200 lines and it could be 50, rewrite it.

Ask yourself: "Would a senior engineer say this is overcomplicated?" If yes, simplify.

### 3. Surgical Changes

**Touch only what you must. Clean up only your own mess.**

When editing existing code:
- Don't "improve" adjacent code, comments, or formatting.
- Don't refactor things that aren't broken.
- Match existing style, even if you'd do it differently.
- If you notice unrelated dead code, mention it - don't delete it.

When your changes create orphans:
- Remove imports/variables/functions that YOUR changes made unused.
- Don't remove pre-existing dead code unless asked.

The test: Every changed line should trace directly to the user's request.

### 4. Goal-Driven Execution

**Define success criteria. Loop until verified.**

Transform tasks into verifiable goals:
- "Add validation" → "Write tests for invalid inputs, then make them pass"
- "Fix the bug" → "Write a test that reproduces it, then make it pass"
- "Refactor X" → "Ensure tests pass before and after"

For multi-step tasks, state a brief plan:
```
1. [Step] → verify: [check]
2. [Step] → verify: [check]
3. [Step] → verify: [check]
```

Strong success criteria let you loop independently. Weak criteria ("make it work") require constant clarification.
