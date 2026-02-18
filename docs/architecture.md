# Architecture Guide for LLM Coding Agents

**Purpose**: Instructions for WHERE to put code and WHAT patterns to follow in Oscil.
**Tech Stack**: C++20 | JUCE 8.0.5 | CMake 3.21+ | OpenGL 3.3 (GLSL 3.30)

---

## Where to Put New Code

```
include/                    # Headers (.h) - public interfaces
├── core/                   # Business logic, state, data models
│   ├── dsp/                # Signal processing headers
│   ├── interfaces/         # Abstract interfaces (I-prefix)
│   └── presets/            # Preset system
├── plugin/                 # Plugin entry points (Processor, Editor)
├── rendering/              # OpenGL visualization
│   ├── effects/            # Post-processing effects
│   ├── shaders/            # 2D waveform shaders
│   └── subsystems/         # Render pipeline components
└── ui/                     # User interface
    ├── components/         # Reusable widgets (OscilButton, etc.)
    ├── dialogs/            # Modal dialogs
    ├── layout/             # Layout management
    └── panels/             # Content panels

src/                        # Implementation (.cpp) - mirrors include/
tests/                      # Unit tests (test_*.cpp)
tests/e2e/                  # E2E Python tests (test_*.py)
resources/shaders/          # GLSL shader files (.vert, .frag)
```

### Decision Tree: Where Do I Put This?

| Creating...             | Location                                                | Naming Pattern            |
| ----------------------- | ------------------------------------------------------- | ------------------------- |
| New UI widget           | `include/ui/components/` + `src/ui/components/`         | `Oscil[Name].h/.cpp`      |
| New dialog              | `include/ui/dialogs/` + `src/ui/dialogs/`               | `[Name]Dialog.h/.cpp`     |
| New post-process effect | `include/rendering/effects/` + `src/rendering/effects/` | `[Name]Effect.h/.cpp`     |
| New waveform shader     | `include/rendering/shaders/` + `src/rendering/shaders/` | `[Name]Shader.h/.cpp`     |
| New core data model     | `include/core/` + `src/core/`                           | `[Name].h/.cpp`           |
| New interface           | `include/core/interfaces/`                              | `I[Name].h`               |
| Unit test               | `tests/`                                                | `test_[snake_case].cpp`   |
| E2E test                | `tests/e2e/`                                            | `test_[snake_case].py`    |
| GLSL shader             | `resources/shaders/`                                    | `[snake_case].vert/.frag` |

---

## Naming Conventions

| Element         | Convention                     | Example                              |
| --------------- | ------------------------------ | ------------------------------------ |
| Classes         | PascalCase                     | `OscilButton`, `BloomEffect`         |
| Interfaces      | I-prefix PascalCase            | `IInstanceRegistry`, `IThemeService` |
| Files           | PascalCase                     | `OscilButton.cpp`, `BloomEffect.h`   |
| Test files      | snake*case with `test*` prefix | `test_oscil_button.cpp`              |
| Namespace       | lowercase                      | `oscil`, `oscil::test`               |
| Private members | trailing underscore            | `enabled_`, `settings_`              |
| Constants       | SCREAMING_SNAKE_CASE           | `DEFAULT_TIMEOUT`                    |

---

## Key Architectural Patterns

### Dependency Injection

All UI components receive dependencies via constructor:

```cpp
// DO: Accept interface reference
OscilButton(IThemeService& themeService);

// DON'T: Use global singleton
OscilButton() : theme_(ThemeManager::getInstance()) {} // BAD
```

### Thread Safety

- **Audio thread**: Lock-free only, no allocations
- **UI thread**: All GUI operations
- Use `std::atomic` for cross-thread state (explicit `memory_order`)
- Use `juce::MessageManager::callAsync()` for UI updates from audio thread

### OpenGL Resource Management

- Always pair `compile()` with `release()`
- Check `isCompiled()` before using shaders
- Use `#if OSCIL_ENABLE_OPENGL` guards

---

## Adding Files to Build

After creating new source files, add them to `cmake/Sources.cmake`:

```cmake
set(OSCIL_SOURCES
    # ... existing files ...

    # UI (components)
    ${CMAKE_SOURCE_DIR}/src/ui/components/Oscil[Name].cpp

    # Rendering (effects)
    ${CMAKE_SOURCE_DIR}/src/rendering/effects/[Name]Effect.cpp
)
```

---

## Common Mistakes

- **Don't**: Use raw pointers for ownership. **Do**: `std::unique_ptr` or `std::shared_ptr`.
- **Don't**: Create global singletons. **Do**: Dependency injection via interfaces.
- **Don't**: Put implementation in headers. **Do**: Separate `.h` from `.cpp`.
- **Don't**: Name test files without `test_` prefix. **Do**: `test_[name].cpp`.
- **Don't**: Forget `#pragma once` in headers.
- **Don't**: Use `using namespace` in headers. **Do**: Explicit `juce::` prefix.
- **Don't**: Allocate memory in audio thread. **Do**: Pre-allocate buffers.
- **Don't**: Skip adding new files to `cmake/Sources.cmake`.

---

## On-Demand References

| Domain                                   | Serena Memory               |
| ---------------------------------------- | --------------------------- |
| Full directory map with key files        | `codebase-structure`        |
| UI component/interface templates         | `juce-component-patterns`   |
| OpenGL shader/effect templates           | `opengl-rendering-patterns` |
| Thread safety patterns, lock-free queues | `thread-safety-patterns`    |
| Test templates and fixtures              | `testing-detailed-patterns` |
