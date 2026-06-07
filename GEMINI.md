# Gemini Instructions for Noteahead

This project follows strict engineering standards. Refer to [Agents.md](./Agents.md) for the foundational architecture, technology stack, and coding principles.

## Additional Mandates

### Unit Testing
- **Lean Headers**: Test headers (`.hpp`) MUST NOT include `QtTest`. They MUST only include `<QObject>`.
- **Granular Includes**: In test source files (`.cpp`), NEVER use `#include <QtTest>`. Use the specific `#include <QTest>` header and other specific utilities (e.g., `<QSignalSpy>`) as needed.
- **MOC Handling**: DO NOT explicitly include `.moc` files (e.g., `#include "test.moc"`) in source files. Rely on CMake's automatic MOC handling by including the header in the `qt_add_executable` source list.
- **Project Structure**: Follow the established pattern for unit tests: each test in its own subdirectory with a `CMakeLists.txt`.

### Coding Style
- **Post-increment**: Always use post-increment (`i++`) instead of pre-increment (`++i`) for loop counters, as specified in `Agents.md`.
- **Initialization**: Use `=` for initializing local normal variables. Use braced initializers `{}` ONLY for class instantiations.
