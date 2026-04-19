# View: main-plugin-layouts

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `main-plugin-layouts.html` with EXACTLY 5 vertically-stacked `.frame` blocks, each 1200 × 760, each labeled with the uppercase variant label below.

## Variants (5 frames, in this order)

1. `SINGLE COLUMN + SIDEBAR EXPANDED` — three panes stacked vertically (Vocals / Drums / Bass), sidebar visible.
2. `TWO COLUMN + SIDEBAR EXPANDED` — 4 panes in 2 × 2 grid (Vocals top-left, Drums top-right, Guitar bottom-left, Bass bottom-right), sidebar visible.
3. `THREE COLUMN + SIDEBAR COLLAPSED` — 6 panes in 3 × 2 grid, sidebar rail mode.
4. `SINGLE COLUMN + HINT TEXT IN STATUS BAR` — same as variant 1 but status bar shows hint text on the left.
5. `SINGLE COLUMN + FPS WARNING STATE` — same as variant 1 but FPS reads `28.4 FPS` in `statusError` color.

## Shell Structure (applies to every frame)

All positions in absolute pixels inside the `.frame`:

- Content viewport: `left: 0; top: 0; right: [sidebar width]; bottom: 22; background: #0A0D12;` — achieved via `width = 1200 - sidebar - 0; height = 738;`.
- Sidebar: `right: 0; top: 0; width: 300` (expanded) or `width: 40` (collapsed); `height: 738; background: #10141B; border-left: 1px solid #1F2630;`.
- Status bar: `left: 0; bottom: 0; width: 1200; height: 22; background: #10141B;`.

## Pane Tile

Pane container: `background: #0F131A; border-radius: 10px; border: 1px solid rgba(232,234,237,0.12);`. Pane inner padding: `0`. Pane is composed of:

- Header bar at top, 32 tall, bg `rgba(232,234,237,0.08)` over pane bg, bottom-border 1px `rgba(232,234,237,0.12)`.
  - Left: 24 × 32 drag handle — render 3 dot-rows of 2 dots each, dot 2 × 2 `#8A94A3 @ 0.5`.
  - Then 8px gap, then pane name bold small (`textPrimary`).
  - Right side: `Processing:` label (`textSecondary`, caption) + mode name in oscillator color (e.g. `Clean` `#1FD4F3`, `Compressed` `#E24BA6`, `Overdrive` `#E87B3A`, `Sub` `#4AD070`). Then 8px, then two icon buttons 24 × 24 (pause/hold, stats). Then 8px, then close icon 24 × 24.
- Waveform canvas fills the rest. Background uses the grid overlay from `_README`. Polyline waveform in the oscillator's assigned color using the `_README` polyline template.
- Bottom 20 tall scrollbar area at bottom of pane: thin 2px `#242A35` track with 80px long thumb `#3B4555` positioned centered horizontally. Right of scrollbar: two 20 × 20 icon buttons (zoom, fullscreen).

## Pane Content per Layout

Variant 1 (3 panes stacked, 1 column):
- Each pane: `left: 16; right: 16; height: 224; top: 16 + index * 232;` (indices 0, 1, 2).
- Pane 1: name `Track 1 - Vocals`, color `#1FD4F3`, processing `Clean`.
- Pane 2: name `Track 2 - Drums`, color `#E24BA6`, processing `Compressed`.
- Pane 3: name `Track 3 - Bass`, color `#4AD070`, processing `Sub`.

Variant 2 (4 panes, 2 × 2):
- Pane slot size: 432 × 348; positions: `(16,16)`, `(464,16)`, `(16,380)`, `(464,380)`.
- Content viewport is narrower here because sidebar expanded = 300 wide, so content width = 900; adjust positions: slot width 434, positions `(16,16) (466,16) (16,380) (466,380)`.
- Pane 1 `Track 1 - Vocals` `#1FD4F3` `Clean`.
- Pane 2 `Track 3 - Drums` `#E24BA6` `Compressed`.
- Pane 3 `Track 2 - Guitar` `#E87B3A` `Overdrive`.
- Pane 4 `Track 4 - Bass` `#4AD070` `Sub`.

Variant 3 (6 panes, 3 × 2, sidebar collapsed → content width = 1160):
- Slot size: 378 × 348; positions row 0: `(16,16) (398,16) (780,16)`, row 1: `(16,380) (398,380) (780,380)`.
- Fill with: `Vocals #1FD4F3 Clean`, `Drums #E24BA6 Compressed`, `Guitar #E87B3A Overdrive`, `Bass #4AD070 Sub`, `Synth #9F70E5 Dry`, `FX #D4B02C Wet`.

Variants 4 and 5 clone variant 1 layout; only the status bar changes (see below).

## Sidebar Content (expanded variants 1, 2, 4, 5)

Top row 40 tall: title area blank left, chevron-left icon button right (24 × 24 centered vertically, 8px from right edge).

Accordion, all three sections expanded:

### Section 1 — OSCILLATORS

- Header 40 tall: chevron-down left (8px from left), caps label `OSCILLATORS` at 16 x-offset from chevron, count badge right (`3`, 18×18 circle bg `rgba(31,212,243,0.20)` text `#1FD4F3`).
- Row below header: full-width primary button `+ Add Oscillator`, height 32, radius 6.
- Oscillator rows (compact, not selected) × 3 — see oscillator-list-states for row spec. In this view, all rows are the default non-hover, non-selected form. Heights 56 each.
  - Row 1: dot `#1FD4F3`, name `Lead Vocal`, track `Track 1`.
  - Row 2: dot `#E24BA6`, name `Drums - Kick`, track `Track 2`.
  - Row 3: dot `#4AD070`, name `Bass DI`, track `Track 3`.
- Bottom 1px divider.

### Section 2 — TIMING

- Header 40 tall: chevron-down, caps label `TIMING`.
- Segmented bar 2 buttons `TIME` / `MELODIC`, `MELODIC` active (bg `rgba(31,212,243,0.15)` text `#1FD4F3`).
- Row: label `Mode` + dropdown `Free Running` (height 32).
- Row: dropdown `1/4 Note` (height 32, full width).
- Row: label `BPM` (30 wide) + numeric field `120.0` + toggle `Sync` with its label, all on one 32-tall row.
- Below: centered `SYNCED` pill (70 × 18 bg `rgba(0,221,0,0.20)` text `#4AD070`).
- Bottom 1px divider.

### Section 3 — OPTIONS

- Header 40 tall: chevron-down, caps label `OPTIONS`.
- Slider row: `Gain` `0.0 dB`.
- Subheader `DISPLAY` 10px caps letter-spacing 0.12em, `textSecondary`.
- Toggle rows: `Show Grid` (on), `Auto-Scale` (off).
- Subheader `LAYOUT`.
- Dropdown `2 Columns`.
- Subheader `THEME`.
- Dropdown `Dark Professional`.
- Subheader `RENDERING`.
- Toggle row: `GPU Acceleration` (on).
- Subheader `CAPTURE QUALITY`.
- Dropdown `Standard (22 kHz)`.
- Dropdown `Medium (5s)` (buffer).
- Toggle row: `Auto-Adjust` (off).

## Sidebar (collapsed — variant 3)

- Width 40. Vertical center: chevron-right icon button 24 × 24.
- Left edge: 4 × 24 resize-grip area centered vertically, 1px vertical lines `rgba(232,234,237,0.08)` at x=1 and x=3.
- No accordion visible.

## Status Bar

- Right cluster (absolute, right-anchored), vertically centered, from right inward with 8px gaps:
  `OpenGL` (80 wide) | `Src: 6` (60) | `Osc: 3` (60) | `Mem: 287.0 MB` (90) | `CPU: 4.2%` (80) | `60.0 FPS` (70).
- FPS color: `#00DD00` for variants 1, 2, 3, 4. In variant 5: text `28.4 FPS` colored `#EE4444`.
- Left zone (variants 1-3): empty background.
- Left zone (variant 4): hint text 12px `textSecondary` at 10px padding, text: `Drag the oscillator header to move it to another pane`.
- Separator between left and right zones when hint present: 1px vertical `#1F2630`, 12px padding on both sides of separator.

## Frame Labels (exact text, above each frame)

- `SINGLE COLUMN + SIDEBAR EXPANDED`
- `TWO COLUMN + SIDEBAR EXPANDED`
- `THREE COLUMN + SIDEBAR COLLAPSED`
- `SINGLE COLUMN + HINT TEXT IN STATUS BAR`
- `SINGLE COLUMN + FPS WARNING STATE`

## Negative Constraints (this file)

- Do NOT add title bar, menu bar, or window chrome — this is the plugin's inner editor component.
- Do NOT add decorative gradients or glow effects behind waveforms.
- Do NOT modify sidebar section order.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] Exactly 5 frames present.
- [ ] Each frame measures 1200 × 760.
- [ ] Sidebar expanded width = 300, collapsed = 40.
- [ ] Status bar height = 22 on every frame.
- [ ] FPS warning state (frame 5) uses `#EE4444` text color.
- [ ] Hint state (frame 4) shows vertical separator between hint and metrics.
- [ ] Every pane has grid overlay + waveform polyline using pane's assigned oscillator color.
