# Oscil

**Multi-track audio oscilloscope plugin for engineers and producers.**

[![Build and Test](https://github.com/OscilAudio/Oscil/actions/workflows/build_and_test.yml/badge.svg)](https://github.com/OscilAudio/Oscil/actions/workflows/build_and_test.yml)
[![License: GPL-3.0](https://img.shields.io/badge/License-GPL--3.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()
[![JUCE 8](https://img.shields.io/badge/JUCE-8.0.12-green.svg)](https://juce.com)

Oscil is a real-time audio visualization platform that lets you analyze multiple audio sources simultaneously. Drop it on any track in your DAW — each instance registers as a signal source, and a single aggregator displays all waveforms together with sample-accurate timing.

## Features

- **Multi-track visualization** — unlimited simultaneous audio sources, rendered in a single view
- **Per-oscillator signal processing** — Stereo, Mono, Mid, Side, Left, Right modes
- **Cross-instance communication** — plugin instances auto-discover each other across DAW tracks
- **OpenGL-accelerated rendering** — GPU-composited waveforms at 60 fps with shader effects (Neon Glow, Gradient Fill, Dual Outline)
- **Post-processing pipeline** — Bloom, Vignette, Film Grain, Chromatic Aberration, Scanlines, Trails, and more
- **Flexible layout** — multi-pane waveform display with drag-and-drop oscillator assignment
- **Theming** — custom color schemes with live preview
- **Timing modes** — time-based and melodic (beat-synced) waveform display

## Formats

| Format | Platform |
|-|-|
| VST3 | macOS, Windows, Linux |
| Audio Unit | macOS |
| CLAP | macOS, Windows, Linux |
| Standalone | macOS, Windows, Linux |

## Building

**Prerequisites:** CMake 3.25+, C++20 compiler (Clang 14+, GCC 11+, MSVC 2019+), Ninja (recommended)

```bash
# Configure and build (Debug)
cmake --preset dev
cmake --build --preset dev

# Run tests
ctest --preset dev

# Release build
cmake --preset release
cmake --build --preset release
```

Build artifacts:
- **VST3:** `build/dev/Oscil_artefacts/Debug/VST3/oscil4.vst3`
- **AU:** `build/dev/Oscil_artefacts/Debug/AU/oscil4.component`
- **Standalone:** `build/dev/Oscil_artefacts/Debug/Standalone/oscil4.app`

See [docs/building.md](docs/building.md) for detailed build instructions and platform-specific notes.

## Architecture

Oscil follows a strict layered architecture enforced by automated lint:

```
plugin/      Entry points (AudioProcessor, AudioProcessorEditor)
  |
ui/          User interface (Components, Panels, Dialogs, Layout)
  |
rendering/   OpenGL visualization (Shaders, Effects, Render Engine)
  |
core/        Business logic (Oscillator, State, Registry, DSP)
```

Higher layers depend on lower layers, never the reverse. See [docs/architecture.md](docs/architecture.md) for details.

## Tech Stack

- **Language:** C++20
- **Framework:** [JUCE 8.0.12](https://juce.com)
- **Graphics:** OpenGL 3.3 / GLSL 3.30
- **Build:** CMake 3.25+ with presets, CPM for dependencies
- **Testing:** GoogleTest (unit), custom HTTP test harness (E2E)
- **CI:** GitHub Actions (macOS, Windows, Linux) with pluginval, code coverage, and RealtimeSanitizer

## Documentation

| Guide | Description |
|-|-|
| [Architecture](docs/architecture.md) | Layer boundaries, class patterns, where to put new code |
| [Building](docs/building.md) | Build presets, dependencies, platform setup |
| [Development](docs/development.md) | Development workflow and conventions |
| [Testing](docs/testing.md) | Unit tests, E2E test harness, test patterns |
| [Shaders](docs/shaders.md) | Adding GPU-accelerated waveform shaders |
| [API](docs/api.md) | Test harness HTTP API reference |

## Built with AI

This project is fully vibecoded using [Claude Code](https://claude.ai/claude-code) (CLI).

## License

This project is licensed under the [GNU General Public License v3.0](LICENSE).
