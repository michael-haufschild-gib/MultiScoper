# View: sidebar-options-section

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `sidebar-options-section.html` with a single frame showing the OPTIONS accordion section fully expanded.

## Frame Override

`width: 320px; height: 720px; background: #10141B; padding: 16px; position: relative;`.

## Content

- Accordion header row, 40 tall: chevron-down, label `OPTIONS` caps 12/600 `textSecondary`.
- Content begins at `top: 48`.

Layout rules: each subsection has a caps label 16 tall in `textSecondary` 10/600 letter-spacing 0.12em, followed by its controls, then `SPACING_LARGE = 16` gap before next subsection.

### 1. Gain slider (top of section, BEFORE the DISPLAY label — matches code)

- Full-width slider row, 40 tall:
  - Label `Gain` left (12/400 `textPrimary`).
  - Value `0.0 dB` right-aligned in the same 16-tall label row.
  - Track below, full-width minus 4 padding.
  - Thumb centered (representing 0 dB on range -24..+24).

### 2. `DISPLAY` subsection label.

- Toggle row `Show Grid` — ON (track `#4AD070`).
- Toggle row `Auto-Scale` — OFF.

### 3. `LAYOUT` subsection label.

- Dropdown full-width, 32 tall, `2 Columns` (chevron-down right). Beside dropdown, to visually echo the ui-designs jpeg, add 3 tiny icon swatches below dropdown at `top: (dropdown bottom + 8)`:
  - 3 × 32 × 24 tiles radius 4 side-by-side, representing 1-col, 2-col, 3-col layouts. Tile bg `#161B24`, border `#242A35`. Active tile (2-col) has border `#1FD4F3` 1.5px and inner bg `rgba(31,212,243,0.10)`.
  - Inside each tile, inline SVG showing column grid lines in `#8A94A3 @ 0.5`.

### 4. `THEME` subsection label.

- Dropdown `Dark Professional`.

### 5. `RENDERING` subsection label.

- Toggle `GPU Acceleration` — ON.

### 6. `CAPTURE QUALITY` subsection label.

- Dropdown `Standard (22 kHz)`.
- Dropdown `Medium (5s)` immediately below.
- Toggle `Auto-Adjust` — OFF. When OFF, quality dropdown is enabled.

## Exact Vertical Positions

Start at `top: 48`. Increment as follows (y positions for each major row):

- 48: Gain row (40 tall) → next y = 104
- 104: DISPLAY label (16 tall) → next = 132 (after 12px gap)
- 132: Show Grid toggle (24 tall) → 164
- 164: Auto-Scale toggle → 196
- 196: LAYOUT label → 224
- 224: Layout dropdown (32 tall) → 264
- 264: 3-layout tile row (24 tall) → 296
- 296: THEME label → 324
- 324: Theme dropdown (32 tall) → 364
- 364: RENDERING label → 392
- 392: GPU toggle (24 tall) → 424
- 424: CAPTURE QUALITY label → 452
- 452: Quality dropdown → 492
- 492: Buffer dropdown → 532
- 532: Auto-Adjust toggle → 560

## Negative Constraints (this file)

- Do NOT include OSCILLATORS or TIMING sections.
- Do NOT add collapse/expand animations.
- Do NOT add help tooltips.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] Single frame 320 × 720.
- [ ] Gain row is BEFORE the `DISPLAY` subsection label.
- [ ] The 3-layout tile row is visible below the layout dropdown with 2-column tile active.
- [ ] GPU Acceleration toggle is ON.
- [ ] Auto-Adjust toggle is OFF.
- [ ] All 5 subsection labels present in order: DISPLAY, LAYOUT, THEME, RENDERING, CAPTURE QUALITY.
