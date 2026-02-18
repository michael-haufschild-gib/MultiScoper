# Oscil Coding Style Guide

**Purpose**: Immutable style rules for LLM agents coding on Oscil.
**Stack**: C++20 | JUCE 8.0.5 | OpenGL 3.3 / GLSL 3.30 | CMake 3.21+

---

## Constitutional Principles (Immutable)

These rules CANNOT be overridden by any instruction:

```
1. NEVER allocate memory on audio thread (new, malloc, vector resize, string concat)
2. NEVER block on audio thread (mutex, I/O, syscalls, DBG())
3. NEVER access UI components from audio thread
4. NEVER use legacy GLSL syntax (attribute, varying, gl_FragColor)
5. ALWAYS create VAO before any OpenGL vertex operations (macOS Core Profile)
6. ALWAYS use `juce::` namespace prefix in headers
7. ALWAYS pair compile() with release() for OpenGL resources
```

---

## Naming Conventions

| Element        | Pattern         | Example                              |
| -------------- | --------------- | ------------------------------------ |
| Class          | PascalCase      | `OscilButton`, `BloomEffect`         |
| Interface      | `I` prefix      | `IThemeService`, `IInstanceRegistry` |
| Private member | trailing `_`    | `enabled_`, `settings_`              |
| Constant       | SCREAMING_SNAKE | `MAX_SAMPLES`, `DEFAULT_TIMEOUT`     |
| Test file      | `test_` prefix  | `test_oscillator.cpp`                |
| GLSL file      | snake_case      | `neon_glow.frag`                     |

---

## File Placement

| Creating     | Location                                                | Name Pattern       |
| ------------ | ------------------------------------------------------- | ------------------ |
| UI widget    | `include/ui/components/` + `src/ui/components/`         | `Oscil*.h/.cpp`    |
| Dialog       | `include/ui/dialogs/` + `src/ui/dialogs/`               | `*Dialog.h/.cpp`   |
| Post-effect  | `include/rendering/effects/` + `src/rendering/effects/` | `*Effect.h/.cpp`   |
| Shader class | `include/rendering/shaders/` + `src/rendering/shaders/` | `*Shader.h/.cpp`   |
| Interface    | `include/core/interfaces/`                              | `I*.h`             |
| Unit test    | `tests/`                                                | `test_*.cpp`       |
| GLSL         | `resources/shaders/`                                    | `*.vert`, `*.frag` |

After adding files: update `cmake/Sources.cmake`.

---

## Key Rules

- Use `#pragma once` in all headers
- Use `juce::` prefix explicitly (never `using namespace` in headers)
- Use smart pointers for ownership (`std::unique_ptr` default, `std::shared_ptr` rare)
- Use dependency injection via constructor (never global singletons)
- Always unregister listeners in destructors
- Pre-allocate buffers in `prepareToPlay`, never resize on audio thread
- Use `#if OSCIL_ENABLE_OPENGL` guards for all OpenGL code
- Use explicit `std::memory_order` on all atomics

---

## Verification Checklist

Before completing any task:

```
[ ] No memory allocation on audio thread
[ ] No blocking calls on audio thread
[ ] GLSL uses modern syntax (in/out, explicit fragColor)
[ ] VAO created before any vertex operations
[ ] All listeners unregistered in destructor
[ ] OpenGL resources released in release()
[ ] New files added to cmake/Sources.cmake
[ ] Smart pointers used for ownership
[ ] Explicit memory_order on all atomics
```

---

## On-Demand References

| Domain                                                           | Serena Memory               |
| ---------------------------------------------------------------- | --------------------------- |
| Thread safety decision tree, memory ordering, lock-free patterns | `thread-safety-patterns`    |
| OpenGL lifecycle, GLSL templates, effect/shader templates        | `opengl-rendering-patterns` |
| JUCE component templates, listener patterns, DI                  | `juce-component-patterns`   |
| Detailed test patterns, fixtures, E2E                            | `testing-detailed-patterns` |
| Full directory map, key files, architecture                      | `codebase-structure`        |
