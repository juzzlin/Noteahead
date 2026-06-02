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
- **Braced Initializers**: Use `{}` for initialization to avoid narrowing conversions and the "most vexing parse".
- **Namespace**: All code belongs in the `noteahead` namespace.
- **Formatting**: Strictly follow the project's `.clang-format` located in the root.
- **Header Guards**: Use `#ifndef FILENAME_HPP` style instead of `#pragma once`.
- **Standard Library**: Prefer `std::` containers and algorithms where appropriate.
- **Post-increment**: Always use post-increment (`i++`) instead of pre-increment (`++i`) for loop counters.

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
