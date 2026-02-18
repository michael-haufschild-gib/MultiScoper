# Oscil

## Identity

Oscil is a professional multi-track oscilloscope audio plugin (VST3, AU, CLAP, Standalone) built with C++20, JUCE 8.0.5, and OpenGL 3.3 for real-time audio visualization across DAW instances.

## Stack

C++20 | JUCE 8.0.5 | CMake 3.21+ | OpenGL 3.3 / GLSL 3.30

## Constraints (Immutable)

| #   | Rule                                                          |
| --- | ------------------------------------------------------------- |
| 1   | Follow `docs/meta/styleguide.md` — no exceptions              |
| 2   | All new features and bug fixes require meaningful tests       |
| 3   | Run `ctest --preset dev` after each fix — iterate until green |
| 4   | Add new source files to `cmake/Sources.cmake` immediately     |
| 5   | No placeholder or TODO code in completed work                 |

## Commands

| Task               | Command                                                                                                                  |
| ------------------ | ------------------------------------------------------------------------------------------------------------------------ |
| Configure          | `cmake --preset dev`                                                                                                     |
| Build              | `cmake --build --preset dev`                                                                                             |
| Run tests          | `ctest --preset dev`                                                                                                     |
| Run specific test  | `ctest --preset dev -R "Pattern"`                                                                                        |
| Clean rebuild      | `rm -rf build/dev && cmake --preset dev && cmake --build --preset dev`                                                   |
| Build test harness | `cmake --preset dev -DOSCIL_BUILD_TEST_HARNESS=ON && cmake --build --preset dev --target OscilTestHarness`               |
| Start test harness | `"./build/dev/test_harness/OscilTestHarness_artefacts/Debug/Oscil Test Harness.app/Contents/MacOS/Oscil Test Harness" &` |
| Run E2E tests      | `cd tests/e2e && python -m pytest -v`                                                                                    |

## Build Outputs

| Output     | Path                                                    |
| ---------- | ------------------------------------------------------- |
| VST3       | `build/dev/Oscil_artefacts/Debug/VST3/oscil4.vst3`      |
| AU         | `build/dev/Oscil_artefacts/Debug/AU/oscil4.component`   |
| Standalone | `build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app` |
| Tests      | `build/dev/OscilTests`                                  |

## Required Reading

@docs/architecture.md
@docs/meta/styleguide.md
@docs/testing.md

## On-Demand Reading

- E2E testing details: `docs/e2e-testing.md`
- Development setup and presets: `docs/development.md`
