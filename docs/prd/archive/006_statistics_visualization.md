
# Feature Requirements: Statistics Visualization

**Status**: Draft
**Priority**: Medium
**Dependencies**: Statistical Analysis Engine

## 1. Overview
This feature handles the visual representation of statistical data calculated by the Analysis Engine. It includes text overlays, graphical envelopes, and warning indicators.

## 2. Requirements

### 2.1 Waveform Overlays
Visual feedback rendered directly into the oscilloscope pane.

- **RMS Envelope**:
  - **Visual**: A filled, semi-transparent area (lighter shade of the waveform color) representing the RMS level over time.
  - **Layering**: Rendered *behind* the main instantaneous waveform.
- **Peak Envelope**:
  - **Visual**: A dashed or dotted line following the peak magnitude over time.
  - **Behavior**: "Peak Hold" style decay (slowly dropping) is often preferred for readability, or true historic peak.
- **Clipping Indicator**:
  - **Trigger**: Any sample in the current block $|x| > 1.0$ (0dBFS).
  - **Visual**:
    - Instant: Red flash of the entire pane background (subtle alpha).
    - Persistent: "Clip" text label turns red for 2-3 seconds.

### 2.2 Corner Information Display
A text-based HUD (Heads-Up Display) in a non-obtrusive corner (e.g., top-right).

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
  - Toggle-able via "Show Statistics" context menu option.

### 2.3 User Interaction
- **Toggle**: Right-click Menu -> "View" -> "Show Statistics".
- **Reset**: Clicking the stats box resets accumulated values (like Max Peak held).
