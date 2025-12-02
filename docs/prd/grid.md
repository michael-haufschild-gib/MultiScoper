# Oscilloscope Pane Grid – User-Facing Requirements

This document describes **what the user sees**, **what the user can do**, and **what should happen** in the oscilloscope pane grid system. It is written from a product/UX perspective rather than as a technical API spec.

The “grid” refers to the background reference lines in each oscilloscope pane that help the user understand **time** (horizontal axis) and **amplitude** (vertical axis).

---

## 1. Overall Experience

### 1.1. When the grid is enabled

- In each pane, behind the waveform(s), the user sees a **subtle grid** made of:
  - **Vertical lines** that represent **time**.
  - **Horizontal lines** that represent **amplitude**.
- A **strong horizontal center line** is always visible when the grid is on. This line represents **zero amplitude**.
- The grid should:
  - Help users understand timing (where beats and milliseconds fall).
  - Help users judge relative level and symmetry of the waveform.
  - Never distract from the waveform itself.

Important:
1. In stereo processing mode, each waveform for the left and right channel has its own grid.
2. When multiple oscillators are assigned to the same pane, each oscillator's waveform(s) use(s) its/their own grid(s). But all grids in the same pane share the same configuration


### 1.2. When the grid is disabled

- When the user turns the grid **off** (using the existing grid toggle):
  - **No grid lines are drawn** in a pane at all.
  - Waveforms appear on a clean background.


---

## 2. Grid Modes and Visual Content

The grid has two conceptual modes based on how the horizontal axis is set up:

1. **Time-based mode (ms / seconds)**
2. **Musical mode (bars / beats)**

The user does not need to choose these explicitly – the mode follows the oscilloscope’s **current timing mode** (time in milliseconds vs melodic interval / note length).

### 2.1. Time-based mode (milliseconds / seconds)

#### What the user sees

- Vertical grid lines divided according to **time**:
  - The visible region covers the user-selected **time window** (for example, 10 ms, 50 ms, 500 ms, 2000 ms, etc.).
  - The grid automatically divides this window into **sensible chunks** (e.g. 1 ms, 2 ms, 5 ms, 10 ms, 50 ms, 100 ms) so that:
    - There are typically **4–10 vertical major lines** visible.
    - The lines are not so close that they become clutter.
- **Major vertical lines** (slightly stronger/clearer):
  - Represent meaningful time positions (e.g. every 5 ms or 10 ms, etc.).
- **Minor vertical lines** (fainter):
  - Optionally show subdivisions between major lines, based on grid density.
- If the UI shows labels (optional):
  - Major lines may be labeled with time values like `0 ms`, `5 ms`, `10 ms`, `50 ms`, `100 ms`, etc., near the top of the pane.

#### How it reacts to user actions

- **When the user changes the visible time window** (e.g. selects a different ms value or zooms in/out):
  - The spacing between vertical lines adjusts automatically.
  - The number of major lines stays reasonable (not too few, not too many).
  - Labels update to the new time values.
- **When the user resizes the pane horizontally** (makes it wider/narrower):
  - The grid keeps showing roughly the same *time* span as before (no time content changes).
  - The grid line positions stretch or compress to fit the new width.
  - If the pane becomes very narrow, the system may reduce the number of visible minor lines to avoid clutter.
- **When the user zooms in/out around a specific region** (if zoom is supported):
  - The grid updates so that the visible portion still has meaningful major time divisions.
  - Labels stay readable as long as there is enough space.

---

### 2.2. Musical mode (bars / beats)

#### What the user sees

- The vertical grid is aligned to **musical time** using the host tempo and time signature (assume at least 4/4 is available):
  - **Strong lines** at **bar boundaries** (e.g. every 4 quarter notes in 4/4).
  - **Medium-strength lines** at **beats** inside the bar (e.g. beat 1, 2, 3, 4).
  - **Very faint lines** for sub-beat subdivisions (e.g. 1/8 or 1/16 notes), depending on grid density.
- If the UI shows labels (optional):
  - Major bar lines may be labeled with bar/beat positions like `1.1`, `1.2`, `1.3`, `1.4`, `2.1`, etc. (`bar.beat`).
  - Somewhere in the pane header or tooltip, the user can see information like:
    - `Grid: Bars & Beats (1/16)`
    - `Tempo: 120 BPM`

#### How it reacts to user actions

- **When the user changes the melodic interval / note length** (e.g. from 1/16 to 1 bar to 4 bars):
  - The visible horizontal range adjusts to match that interval exactly (e.g. exactly 1 bar or 2 bars).
  - The grid lines reposition to ensure:
    - Clear separation of bars.
    - Visible beats within each bar.
    - Subdivisions only if there is space.
- **When the host tempo changes**:
  - The **time** between beats changes, but the **visual alignment** of bars and beats within the pane remains correct for the new tempo.
  - The user still sees 1 bar / 2 bars / 4 bars, etc. but the underlying time length has changed.
- **When the pane is resized horizontally**:
  - The visible bar/beat span remains the same (e.g. still 2 bars), but the spacing of lines changes to fit the new width.
  - If the pane becomes very narrow, the system may hide sub-beat lines to keep the main bar & beat structure readable.

---

### 2.3. Vertical amplitude reference

#### What the user sees

- A **prominent horizontal center line** that represents **zero amplitude**.
  - This line is always visible whenever the grid is enabled.
  - It should look slightly stronger or more distinctive than other horizontal lines.
- Additional **horizontal reference lines** above and below center:
  - At least the “top” and “bottom” of the pane typically represent **+1.0** and **−1.0** (full-scale normalized amplitude).
  - One or more lighter lines may be placed at intermediate levels (e.g. ±0.5) to help with visual judgment.

Optional behaviors:

- If a more advanced view is enabled (e.g. dB mode), horizontal lines can represent meaningful dB levels (0 dBFS, −6 dB, −12 dB, etc.).

#### How it reacts to user actions

- **When the user zooms vertically** (if supported) or when the amplitude scale changes:
  - The zero line stays in the center.
  - Horizontal reference lines reposition according to the new scale.
- **When the pane is resized vertically**:
  - All horizontal lines are stretched / compressed to fit the new height.
  - The zero line remains exactly at the vertical center.

---

## 3. Grid Appearance and Clarity

### 3.1. Visual style

- Grid lines are always **behind** the waveforms and any visual effects.
- The grid’s color and opacity should make it:
  - Clearly visible as a reference.
  - Not overpowering the waveform (the waveform should always be the main focus).
- **Major lines** (bar boundaries, main time markers, zero line) are:
  - Slightly thicker and/or higher contrast.
- **Minor lines** (sub-beat subdivisions, intermediate amplitude lines) are:
  - Thinner, fainter, can be dashed or simply low-opacity.

If the user chooses a **dark theme** or **light theme**, the grid should adapt so that lines always have enough contrast with the background but still look subtle.

### 3.2. Grid density

If a grid density option is present or added (Minimal/Normal/Dense):

- **Minimal**:
  - Only the most important things: zero amplitude line, a few main vertical divisions (bars or large ms steps).
- **Normal** (default):
  - Zero line, major vertical lines, and some minor verticals where helpful.
- **Dense**:
  - Full bar + beat + subdivision grid (in musical mode) or more time subdivisions (in ms mode), as long as they don’t crowd each other.

When the pane becomes too small, the grid should **automatically lean towards Minimal behavior** (fewer minor lines) to avoid clutter.

---

## 4. User Controls and What They Do

### 4.1. Grid on/off (existing setting)

- **Control**: `Grid: On / Off` (already present in the plugin).
- **When the user turns the grid ON**:
  - The grid appears in all relevant panes according to the current timing and amplitude settings.
- **When the user turns the grid OFF**:
  - The grid disappears completely in the affected panes.
  - Waveforms and effects remain unchanged.
  - Subsequent zoom, resize, or mode changes do not cause the grid to reappear until the user turns it back on.

### 4.2. Timing mode changes (ms / musical)

The user doesn’t toggle “grid mode” directly. Instead, they choose **how the oscilloscope displays time** in the sidebar's timing section panel, and the grid follows:

- If the user selects a **time in ms** (e.g. `10 ms`, `200 ms`, `1000 ms`):
  - The grid immediately switches to **time-based mode**.
  - Vertical lines represent **actual time**.
- If the user selects a **musical interval** (e.g. `1/16 note`, `1 bar`, `4 bars`):
  - The grid switches to **musical mode**.
  - Vertical lines represent bars, beats, and sub-beats.

The transition between modes should be smooth – the user should just see the grid reorganize itself to be musically meaningful.

### 4.3. Pane resize

- When a pane resizes:
  - The grid automatically “reflows” to fit the new width and height.
  - No manual refresh is needed; the change should feel immediate.
  - If the pane becomes extremely small, the grid may hide some minor lines to preserve clarity.


---

## 5. Edge Cases and Special Scenarios

### 5.1. Extremely small panes

**Scenario:** A pane is reduced to a very small size (e.g. tiny thumbnail).

**Expected behavior:**

- The grid must avoid clutter:
  - Show only the most essential lines: zero amplitude, maybe a couple of main vertical divisions.
  - Hide minor lines and most labels (if any).
- Waveform should remain visible and clear; the grid should not obscure it with too many lines.

### 5.2. Very large or very small time windows

**Scenario A:** The user selects a very **short** time window (e.g. 0.5 ms).
**Scenario B:** The user selects a very **long** time window (e.g. several seconds or multiple bars).

**Expected behavior:**

- The grid always keeps the number of main vertical divisions to a reasonable range (roughly 4–10 major lines):
  - For very short time windows, divisions get smaller (e.g. 0.1 ms, 0.2 ms, etc.), but not so small that they overlap on screen.
  - For very long windows, divisions get larger (500 ms, 1 s, 2 s, etc.), so that labels and lines remain readable.
- If the chosen time window would make grid lines appear closer than a few pixels, minor lines should not be drawn at all.

### 5.3. Missing or invalid tempo in musical mode

**Scenario:** The plugin is in a musical timing mode (bars/beats), but the host does not provide valid tempo or transport information (e.g. tempo=0, “no host”, or a non-tempo-capable environment).

**Expected behavior:**

- The grid should **fall back gracefully**:
  - Option 1 (preferred): Treat the current musical interval as a fixed time window using a default tempo (e.g. 120 BPM), and document this behavior.
  - Option 2: Temporarily revert to a **time-based grid** matching the underlying time span of the selected interval, and show a small visual hint (e.g. “No tempo – using time grid”).

- Under no circumstances should the grid:
  - Crash the plugin.
  - Flicker or show nonsense values (e.g. infinite lines, NaN positions).

### 5.4. Host time signature changes

**Scenario:** When in musical mode, the host changes the time signature (e.g. from 4/4 to 3/4).

**Expected behavior:**

- The bar/beat grid updates to reflect the new beats per bar:
  - The number of beats per bar and the bar length changes in the grid.
  - The user now sees bar boundaries appropriate for the new time signature.
- Transitions should not cause jittery or flickering redraws; a single smooth update is sufficient.

### 5.5. Vertical scale or amplitude configuration errors

**Scenario:** An unexpected amplitude configuration occurs (e.g. minAmplitude == maxAmplitude due to a bug or data issue).

**Expected behavior:**

- The grid should avoid invalid operations (e.g. division by zero):
  - If the amplitude range collapses, fallback to a sensible default range, such as −1.0 to +1.0.
- User-facing behavior:
  - The zero line should still be visible and centered.
  - The waveform may look “squashed” or flat, but the plugin should remain stable.

### 5.6. Performance constraints

**Scenario:** The user has many panes, high-density grid, and a heavy shader/particle effect active.

**Expected behavior:**

- The grid should remain **lightweight** and **not dominate rendering cost**.
- If needed, the plugin can internally reduce grid detail dynamically in heavy-load situations (for example, dropping minor lines) to maintain UI responsiveness.
- From the user’s perspective, the grid should never cause lag or freeze; at worst, they see a simpler grid when the load is high.

---

## 6. Error Handling and Visual Consistency

### 6.1. General robustness

In all error or unexpected situations (missing host data, invalid parameters, extreme sizes):

- The plugin should **never crash** or display broken visuals.
- The grid may simplify itself or fall back to a safe default, but:
  - Waveforms should remain visible.
  - The zero line should remain in the center when possible.
  - The user should still be able to interact with panes (resize, zoom, toggle grid, etc.).

### 6.2. Respecting user choice

- Once the user turns the grid off, it must stay off until they turn it on, regardless of other mode or view changes.
- Once turned back on, the grid should immediately reflect the current timing mode, tempo, pane size, and zoom without requiring the user to do anything else.

---

## 7. Summary for UX

- The grid is a **visual helper**, not the star of the show.
- It should always be:
  - **Meaningful** (aligned with time or musical structure).
  - **Readable** (not overcrowded with lines or labels).
  - **Responsive** (updates instantly when the user changes settings or resizes panes).
- In failure or edge cases, the grid should **fail gracefully**, simplify itself, or temporarily fall back to a default rather than breaking the UI.

These requirements should give a coding LLM enough behavioral guidance to implement the grid in a way that feels intuitive and reliable from a user’s perspective.
