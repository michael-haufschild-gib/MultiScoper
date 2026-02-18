# Waveform Hover Tooltip – User-Facing Requirements

This document describes **what users should see in the tooltip when hovering over a waveform**, and **how the tooltip should behave** in different situations. It is written from a UX/behavior perspective, not as code.

The goal is to provide **quick, precise information** about the waveform at the mouse position without overwhelming the user or obscuring the view.

---

## 1. When the Tooltip Appears

### 1.1. Basic behavior

- When the user moves the mouse **over a waveform area in a pane** and holds it there briefly (e.g. ~200 ms), a **tooltip appears** near the cursor.
- The tooltip follows the cursor **horizontally** and **vertically** with a small offset, so it doesn’t cover the exact sample under the cursor.
- The tooltip only appears if:
  - The pane is visible.
  - At least one waveform is visible at the mouse position.
  - Tooltips are enabled (if there is a global “show tooltips” setting).

### 1.2. When the tooltip disappears

- The tooltip disappears when:
  - The mouse leaves the pane.
  - The user clicks outside the pane.
  - The pane is hidden, collapsed, or removed.
- Optional behavior: the tooltip can fade out with a very short fade to avoid flicker.

---

## 2. What the Tooltip Shows (General Content)

When hovering over a waveform, the tooltip should answer **three main questions** for the user:

1. **Where in time am I?**
2. **What is the amplitude here?**
3. **Which signal / channel am I looking at?**

### 2.1. Time information

- The tooltip must show the **time position** corresponding to the cursor’s X position within the pane.
- The time format depends on the pane’s **current timing mode**:
  - **Time-based mode (ms / s):**
    - Show time as `X.XX ms` or `X.XXX s` based on magnitude.
    - Example: `Time: 7.25 ms` or `Time: 1.203 s`
  - **Musical mode (bars / beats):**
    - Show position as `bar.beat + fractional beat`, if possible.
    - Example: `Pos: 2.3.25` (bar 2, beat 3, 25% into the beat).
    - Optionally, show the equivalent time in ms underneath or on the next line.
- If the plugin is not receiving tempo or transport information in musical mode, time should fall back to **plain ms** and the tooltip may show a small hint like `Tempo unavailable – showing ms`.

### 2.2. Amplitude information

- The tooltip must show the **amplitude** at the cursor’s Y position for the hovered waveform(s).
- Format will depend on view mode:
  - In **normalized amplitude view**:
    - Show a normalized value in the range −1.0 to +1.0 with reasonable precision.
    - Example: `Amp: −0.34`
  - If a **dB view** or level overlay is active:
    - Show both normalized amplitude **and** approximate dBFS if available.
    - Example: `Amp: −0.34 (~−9.4 dBFS)`
- If no valid sample data is available at the hovered position (e.g. fully silent, or out-of-buffer region):
  - Show amplitude as `0.00` or display `Amp: 0 (silent)`.

### 2.3. Signal identity & mode

The tooltip must indicate **which waveform / channel / mode** the user is inspecting:

- Always show the **waveform name or source label**, if available (e.g. track name, alias, or internal ID):
  - Example: `Source: Bass Bus`
- Always show the **processing mode**:
  - `Mode: Stereo`
  - `Mode: Mono`
  - `Mode: Mid`
  - `Mode: Side`
  - `Mode: Left`
  - `Mode: Right`
- For each waveform, show a small **color indicator** (swatch, thin bar, or dot) matching the waveform’s color in the pane so the user can visually connect tooltip information to the line on screen.

---

## 3. Behavior per Processing Mode

The oscilloscope supports the following processing modes for each waveform:

- **Stereo** (shows two waveforms: Left and Right)
- **Mono**
- **Mid**
- **Side**
- **Left**
- **Right**

### 3.1. Stereo mode (two waveforms)

When the waveform is in **stereo mode**, the pane shows **two waveforms** (Left and Right). The tooltip should carefully differentiate between them.

#### 3.1.1. What the user sees

- On hover, the tooltip shows:
  - **Shared time position** (as described in section 2.1).
  - For the stereo waveform:
    - `Source: <Waveform Name>`
    - `Mode: Stereo`
    - `Channel L: <amp value>`
    - `Channel R: <amp value>`
  - Example:
    - `Source: Drum Bus`
    - `Mode: Stereo`
    - `L: −0.78 (~−2.2 dBFS)`
    - `R: −0.65 (~−3.7 dBFS)`
- The L and R lines in the tooltip should use small color indicators that match the colors used in the pane for L/R (if different).

#### 3.1.2. Which channel is “primary” at the cursor

When the user hovers near the two waveforms:

- If the cursor is **closer to the Left channel curve**, the tooltip may highlight the Left value slightly (e.g. bold text or a small marker).
- If closer to the Right channel curve, the Right value is highlighted.
- If both are very close (near overlap), both are treated equally; no special highlighting is required, or both can be shown the same.

This behavior helps users understand which line they are hovering on without hiding the other channel information.

---

### 3.2. Mono, Mid, Side, Left, Right modes (single waveform)

For these modes, the plugin displays **one waveform curve** for the processed signal.

#### What the user sees

- This is similar to the general case, but with the mode clearly indicated:
  - `Source: <Waveform Name>`
  - `Mode: Mono` (or `Mid`, `Side`, `Left`, `Right`)
  - `Amp: <value>` (plus optional dB)
- Example for a Side waveform:
  - `Source: Pad FX Bus`
  - `Mode: Side`
  - `Amp: 0.12 (~−18.4 dBFS)`

Even though only one curve is visible, the mode is critical so that users understand what they are looking at (e.g. “Side” rather than “Right”).

---

## 4. Multiple Waveforms in the Same Pane

A pane can display **multiple waveforms from different tracks/sources**. The tooltip must behave sensibly when several waveforms are near the cursor.

### 4.1. Hovering near multiple waveforms

**Scenario:** There are 2+ waveforms in a pane; the mouse is at a position where several curves are close to each other vertically.

**Expected behavior:**

- The tooltip should prioritize the **closest waveform** to the cursor as the **primary** waveform.
- The tooltip shows information for:
  - The primary waveform (always).
  - Optionally, up to a small number of additional nearby waveforms (e.g. up to 3), listed underneath.

Example layout:

- First line block – primary waveform (closest to cursor):
  - Color swatch
  - `Source: Kick`
  - `Mode: Stereo`
  - `L: −0.82 (~−1.7 dBFS)`
  - `R: −0.79 (~−2.0 dBFS)`
- Additional block(s) – secondary waveforms (sorted by distance from cursor):
  - Color swatch
  - `Source: Bass`
  - `Mode: Mono`
  - `Amp: −0.45 (~−6.9 dBFS)`

### 4.2. Visual distinction between multiple entries

- Each waveform entry in the tooltip should have:
  - A **color indicator** that matches its curve in the pane.
  - Clear separation from other entries via spacing or dividers.
- The primary waveform can be visually emphasized (e.g. bold source name or a slightly stronger background in that section).

### 4.3. Limiting information for clarity

- To avoid clutter, the tooltip should limit the number of waveforms shown:
  - Show the **primary** waveform plus, at most, a few closest neighbors.
  - If more waveforms are present further away, they can be omitted from the tooltip.
- If this limit is reached, the tooltip may show a short info line like:
  - `(+3 more waveforms)`

---

## 5. Behavior with Playback, Pause, and Freeze

### 5.1. During live playback

- When the oscilloscope is scrolling / updating in real time:
  - The tooltip should follow the cursor in the **current frame**.
  - Time and amplitude values should update smoothly as the waveform moves beneath the cursor.
- To avoid excessive flicker or performance issues, tooltip updates should be **visually stable** even if audio is high-frequency.

### 5.2. When the display is frozen or paused

- If the user freezes the display (e.g. via a “freeze” button or when playback stops):
  - The waveform stops moving.
  - The tooltip still works as normal, but values no longer change over time until playback resumes or freeze is released.
- Hovering in a frozen state should feel like inspecting a static picture with precise readouts.

### 5.3. No data yet / empty buffer

**Scenario:** The pane has been created, but there has not yet been any audio captured or displayed, so the waveform is essentially empty.

**Expected behavior:**

- If the user hovers over an area with **no waveform samples**:
  - Either no tooltip appears, or the tooltip shows a very minimal message such as:
    - `No data`
- As soon as audio is displayed in the pane, tooltip behavior switches to normal.

---

## 6. Tooltip Placement and Styling

### 6.1. Position relative to cursor

- The tooltip should appear **near** the cursor but not covering the exact point of interest:
  - Typically offset slightly to the **top-right** or **bottom-right** of the cursor.
- If the cursor is close to the right or bottom edge of the pane:
  - The tooltip should automatically flip to appear on the **left** or **above** the cursor to stay fully visible.

### 6.2. Appearance

- Background:
  - Semi-transparent, high-contrast with pane background (e.g. dark translucent box on a dark theme, but still readable).
- Text:
  - Clear, legible font.
  - Use small but readable size; avoid large, dominant text.
- Color indicators:
  - Small square or rounded swatch using the waveform’s assigned color.

### 6.3. Interaction with shaders and effects

- Shaders, glow, and particle effects should **not obscure the tooltip**.
- The tooltip is part of the UI layer and should always draw **on top** of waveform visuals while visible.

---

## 7. Edge Cases and Error Scenarios

### 7.1. Invalid mode or unknown configuration

**Scenario:** Due to a bug or unsupported configuration, the waveform’s mode is not recognized.

**Expected behavior:**

- Tooltip should still show **time** and **amplitude**.
- For the mode line:
  - Show `Mode: Unknown` or omit the mode line.
- The plugin should not crash or show garbage text.

### 7.2. Extremely high zoom / dense sampling

**Scenario:** At very high zoom levels, the line under the cursor may represent very few samples, or the waveform may look jagged.

**Expected behavior:**

- Tooltip still shows the sample-based amplitude at the cursor position.
- If multiple samples correspond to exactly the same pixel column, it is acceptable to use the **nearest** or a representative sample.
- There is no need to show per-sample indices to users in the tooltip; keep it musically and visually focused.

### 7.3. Performance constraints

**Scenario:** The user has many panes, many waveforms, and heavy rendering effects. Tooltips might add some overhead.

**Expected behavior:**

- Tooltip computation must be light enough to not noticeably degrade UI performance.
- If needed, the system can:
  - Throttle tooltip update frequency slightly (e.g. not updating more than every frame or two).
  - Limit the number of waveforms shown in the tooltip more aggressively under heavy load.
- From the user’s perspective, the tooltip may feel slightly less “live” under heavy load, but it should **never lag badly** or cause UI freezes.

---

## 8. UX Summary

- Hover tooltips are a **precision inspection tool**:
  - They give exact time, amplitude, and channel/mode information at the current cursor position.
- They adapt naturally to the oscilloscope’s current modes:
  - Time vs musical grid.
  - Stereo vs Mono/Mid/Side/Left/Right processing modes.
- They respect multi-waveform setups:
  - Show the closest waveform as the primary entry.
  - Optionally show a few nearby waveforms with color-coded entries.
- They are always:
  - **Non-intrusive** (small, semi-transparent, offset from the cursor).
  - **Readable** (clear labels, consistent structure).
  - **Robust** (handle no data, unknown modes, and performance edge cases gracefully).

These requirements should give a coding LLM enough behavioral guidance to implement waveform hover tooltips in a way that feels intuitive and helpful to users across all supported processing modes.
