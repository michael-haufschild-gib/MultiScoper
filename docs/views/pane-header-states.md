# View: pane-header-states

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `pane-header-states.html` with 5 stacked frames showing each pane-header state. Each frame = one pane.

## Frame Override

Each frame: `width: 900px; height: 240px; background: #0A0D12; position: relative; padding: 16px;`.

## Pane Structure (applied inside each frame)

Pane container: `left: 16; top: 16; width: 868; height: 208; background: #0F131A; border: 1px solid rgba(232,234,237,0.12); border-radius: 10;`.

Inside pane, header bar: `left: 0; top: 0; width: 868; height: 32; background: rgba(232,234,237,0.08); border-bottom: 1px solid rgba(232,234,237,0.12);`.

Header bar contents (left to right):
- 24 × 32 drag handle (3 dot rows of 2 dots, 2 × 2 `#8A94A3 @ 0.5`).
- 8px gap.
- Pane name 12/600 `textPrimary`.
- Flex-filler to push right cluster.
- Right cluster (from right edge, 8px padding):
  - Close icon 24 × 24 (`x` SVG).
  - 4px gap.
  - Stats icon button 24 × 24 (`bar-chart-3`).
  - 4px gap.
  - Hold/play icon button 24 × 24 (`pause` when NOT held, `play` when held).
- Left of right cluster (same row): processing-mode info — `Processing:` caption `textSecondary` 10/400 + processing-mode pill 60 × 18 radius 999 + optional `+N more` caption.

Inside pane, waveform area: `left: 0; top: 32; width: 868; height: 176; background: #0F131A;` — grid + waveform polyline per `_README`.

## Frame Definitions

### Frame 1 — EMPTY PANE (no oscillator assigned)

Label: `EMPTY PANE`.

- Pane name: `Pane 1`.
- Right cluster: NO processing pill. Instead, a caption `No oscillators assigned` in `textMuted` right of the name.
- Waveform area: NO polyline. Grid is still visible but very faint (`#242A35 @ 0.4`). Centered in waveform area: `Drag an oscillator here` 13/400 `textMuted` with a 32 × 32 `plus` SVG above it.

### Frame 2 — SINGLE OSCILLATOR

Label: `SINGLE OSCILLATOR ASSIGNED`.

- Pane name: `Pane 1`.
- Processing pill `STEREO` bg `rgba(31,212,243,0.20)` text `#1FD4F3`. `Processing:` caption precedes it.
- No `+N more` caption.
- Waveform area: polyline in `#1FD4F3` stereo waveform (two polylines offset ±40px).
- Hold button showing `pause` icon (default, not held).
- Stats button NOT active (icon alpha 1.0, no bg).

### Frame 3 — MULTI-OSCILLATOR (PRIMARY + N MORE)

Label: `MULTI-OSCILLATOR (PRIMARY + 2 MORE)`.

- Pane name: `Pane 2`.
- Processing pill `MONO` bg `rgba(226,75,166,0.20)` text `#E24BA6`.
- Caption after pill: `+2 more` 10/400 `textSecondary`.
- Waveform area: 3 polylines layered, at colors `#E24BA6`, `#4AD070`, `#E87B3A`, each 60% stroke opacity, different vertical offsets.

### Frame 4 — HOLD TOGGLED (PAUSED)

Label: `HOLD TOGGLED — DISPLAY PAUSED`.

- Pane name: `Pane 1`.
- Processing pill `STEREO` (same as frame 2).
- Hold button now shows `play` icon inside a 1px border `#1FD4F3` box (the "toggled" state per PaneActionBar).
- Bar bg: `rgba(31,212,243,0.06)` subtle tint across the whole pane header to indicate paused.
- Waveform area: waveform frozen — render the same polyline as frame 2 but at full opacity and overlay a semi-transparent pause indicator in the top-right corner of the waveform area: 24 × 24 pause icon inside a `rgba(0,0,0,0.50)` rounded-4 box at 16px from top-right.

### Frame 5 — STATS OVERLAY VISIBLE

Label: `STATS OVERLAY ON — PEAK & RMS VISIBLE`.

- Same as frame 2 baseline.
- Stats icon button toggled ON: bg `rgba(31,212,243,0.15)` inside the 24 × 24 button.
- Waveform area has an overlay panel at top-right: `right: 16; top: 16; width: 220; height: 68; background: rgba(15,19,26,0.90); border: 1px solid #242A35; border-radius: 8; padding: 12;`.
- Overlay contents:
  - Row 1: `PEAK` 9/600 caps `textSecondary` left, `-3.2 dB` 13/600 `textPrimary` right.
  - Row 2: `RMS` 9/600 caps `textSecondary` left, `-14.8 dB` 13/600 `textPrimary` right.
  - Row 3: small horizontal meter bar 196 × 4 (peak meter gradient), peak hold line at 78%.

## Negative Constraints (this file)

- Do NOT add "maximize" or "split" buttons — not in current action bar.
- Do NOT show pane resize handles between panes; this mockup isolates single panes.
- Do NOT include the full plugin shell.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] 5 frames, each 900 × 240.
- [ ] Frame 1 has NO processing pill and empty-waveform-area instruction.
- [ ] Frame 2 uses stereo `#1FD4F3` color.
- [ ] Frame 3 shows `+2 more` caption and 3 layered polylines.
- [ ] Frame 4 hold button shows `play` icon with accent border (toggled state).
- [ ] Frame 5 shows stats overlay with PEAK and RMS rows and a meter bar.
