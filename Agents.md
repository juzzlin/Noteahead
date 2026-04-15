# Noteahead: Agent Guide

Noteahead is a MIDI tracker and sequencer for Linux, written in **Qt/QML** and **C++20**. It follows modern software engineering practices, prioritizing performance, accuracy, and maintainability.

## 🏗 Architecture

The project follows a layered architecture to ensure separation of concerns:

- **`src/domain`**: The "Heart" of the application. Contains pure business logic and data structures (e.g., `Song`, `Track`, `Pattern`, `Arpeggiator`). It should remain as independent of the UI and infrastructure as possible.
- **`src/application`**: The "Brain". Orchestrates domain objects and provides services for the UI.
    - **`services/`**: High-level logic (e.g., `MidiService`, `EditorService`).
    - **`models/`**: `QAbstractListModel` and other QML-facing models.
    - **`command/`**: Implementation of the Command Pattern for Undo/Redo functionality.
- **`src/infra`**: The "Hands". Handles external systems:
    - **`midi/`**: RtMidi backend and MIDI file export/import.
    - **`audio/`**: RtAudio backend and audio recording.
    - **`video/`**: ffmpeg-based video generation.
    - **`settings/`**: Persistent configuration management.
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

### C++ Specifics
- **Braced Initializers**: Use `{}` for initialization to avoid narrowing conversions and the "most vexing parse".
- **Namespace**: All code belongs in the `noteahead` namespace.
- **Formatting**: Strictly follow the project's `.clang-format` located in the root.
- **Header Guards**: Use `#ifndef FILENAME_HPP` style instead of `#pragma once`.
- **Standard Library**: Prefer `std::` containers and algorithms where appropriate.

### Qt & QML
- **Model/View**: Keep logic in C++ models and services; QML should only handle presentation.
- **Properties**: Use `Q_PROPERTY` with `READ`, `WRITE`, and `NOTIFY` signals correctly.
- **Signals/Slots**: Prefer the modern `connect` syntax (`&Sender::signal, receiver, &Receiver::slot`).

## 🧪 Testing Strategy

- **Unit Tests**: Located in `src/unit_tests`. Every new feature or bug fix should include a test.
- **Execution**: Run tests using `ctest` from the build directory.
- **Framework**: Uses the Qt Test framework.

## 🚀 Build & Test

- **Build**: Use `cmake --build build` to build the project.
- **Run Tests**: Use `ctest --test-dir build` to run all tests.

## 🚀 Performance & Timing

- **Accuracy**: Noteahead renders events just before playback to ensure jitter-free, drift-free timing.
- **Thread Safety**: The player runs in a dedicated thread (`PlayerWorker`). Use safe synchronization primitives when interacting between the UI and player threads.
- Always use post-increment (i++) instead of pre-increment (++i) for loop counters.
