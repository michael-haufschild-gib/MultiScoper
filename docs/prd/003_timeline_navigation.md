# Feature Requirements: Timeline & Navigation

**Status**: Draft
**Priority**: High
**Dependencies**: History System

## 1. Overview
The Timeline & Navigation feature provides the user interface to interact with the History System. It allows users to "freeze" the display, scrub through past time, and visually correlate waveforms with a timeline.

## 2. Requirements

### 2.1 Functional Requirements
- **Freeze/Pause**:
    - Users can toggle a "Freeze" mode.
    - When Frozen: Display stops updating with live data and shows the buffer state at the moment of freeze.
    - When Unfrozen: Display snap-reverts to monitoring live input (Real-time).
- **Scrubbing**:
    - Users can scroll backward in time to view any section of the History Buffer.
    - Scrolling automatically engages "Freeze" behavior (or "History View" mode).
- **Timeline Visualization**:
    - A visual scrubber bar located below the main waveform display.
    - Indicators for "Now" (Live head) and "View" (Current rendered window).
- **Zoom**:
    - Ability to zoom in/out of the timeline to navigate long histories (120s) vs short transients (100ms).
- **Sync with DAW**:
    - "Freeze at Playhead": Optional mode where the history view is linked to the DAW's transport position (if provided by host).

### 2.2 User Interface
- **Scrubber Bar**:
    - Full-width interactive bar at the bottom of the main view.
    - Minimap representation of signal presence (amplitude envelope) is desirable but optional for V1.
- **Transport Controls**:
    - Play/Pause (Live/Freeze) toggle button.
    - "Jump to Live" button.
- **Export Range**:
    - Ability to export the currently visible time range (or selected range) to a file (WAV/CSV).

### 2.3 Rendering Implications
- **Decoupled Render Time**:
    - The renderer needs a `RenderTime` or `ViewOffset` parameter.
    - Shaders must accept a start index/offset into the History Buffer instead of just rendering "current block".
