# Feature Requirements: Statistics Visualization

**Status**: Draft
**Priority**: Medium
**Dependencies**: Statistical Analysis Engine

## 1. Overview
This feature handles the display of statistical data calculated by the Analysis Engine in a pane's text overlay.

## 2. Requirements

### 2. Corner Information Display
1. A text-based HUD (Heads-Up Display) in the top-right corner of a pane.
2. Content displays data per oscillator assigned to the pane using a table where each oscillator gets its own column (column title is the abbreviated name of the oscillator)

- **Content**:
  | Metric | Format | Update Rate |
  | :--- | :--- | :--- |
  | RMS | `-14.5 dB` | ~15Hz (smoothed) |
  | Peak | `-1.2 dB` | Instant |
  | Crest | `13.3 dB` | ~5Hz |
  | DC | `0.5%` | ~5Hz (Only if >0.1%) |
  | Attack | `12 ms` | On Event |
  | Decay | `250 ms` | On Event |

- **Design**:
  - Monospace font for digits to prevent jitter.
  - "Ghost" text for labels, bright text for values.

3. Overlay has an icon button in the top right which uses the icon resources/icomoon/SVG/redo.svg. When clicked it resets accumulated values for all oscillators assigned to the pane (like Max Peak held).

4. User can select text for copy&pasting in the stats overlay
