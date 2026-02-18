# Feature Requirements: Persistence & Afterglow

**Status**: Draft
**Priority**: Medium
**Related To**: History System

## 1. Overview
The Persistence (or "Afterglow") feature allows users to visualize the recent path of the oscilloscope beam, creating a fading trail effect. This emulates phosphor persistence on analog oscilloscopes and helps visual coherence for fast-moving signals.

## 2. Requirements

### 2.1 Functional Requirements
- **Configurable Decay**: The duration of the afterglow must be adjustable by the user.
    - Range: 0.0s (No persistence) to 2.0s (Long persistence).
    - Default: 0.1s.
- **Visual Style**: The trail must fade opacity linearly or exponentially over time.
- **Color Consistency**: The trail must match the color of the parent waveform (or be a derived shade).
- **Per-Oscillator Control**: (Optional) While global persistence is acceptable for MVP, per-oscillator persistence is preferred if UI space permits.
- **Performance**: Enabling persistence must not degrade rendering performance below 60fps on reference hardware (M1/M2 chips).

### 2.2 Technical Constraints
- Must utilize GPU acceleration (OpenGL).
- Must avoid CPU-side buffer accumulation to ensure performance.
- Should reuse existing `TrailsEffect` infrastructure if applicable.

### 2.3 User Interface
- **Global Control**: A slider labeled "Persistence" or "Afterglow" in the main toolbar or view settings.
- **Feedback**: Immediate visual feedback on the active waveform display.
