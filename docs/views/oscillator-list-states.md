# View: oscillator-list-states

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `oscillator-list-states.html` containing 6 stacked state variants of a single oscillator list row, each rendered inside its own `.frame` override. Focus is on the row, not the full plugin — use smaller frames.

## Frame Override

Each variant lives in a `.frame` of `width: 520px; height: 120px; background: #10141B; padding: 16px; position: relative;`. The sidebar column bg is `#10141B`.

Variant 3 (selected-expanded) needs a taller frame: `height: 160px`.

Variant 6 (dragging ghost) needs `height: 180px` to show the source row plus ghost.

## Row Anatomy (base template applied to all variants)

Row container: `width: 488; position: relative; border-radius: 8;`. For non-selected rows: `height: 56`. For selected: `height: 96` (with processing-mode segmented bar beneath).

From left, absolute positions inside the row:

- `left: 0; top: 16; width: 24; height: 24;` — drag handle SVG (2 columns of 3 dots; Lucide `grip-vertical`). Stroke `#8A94A3 @ 0.5` unless variant says hover → `#8A94A3 @ 1.0`.
- `left: 28; top: 20; width: 16; height: 16;` — color dot, radius 999, filled with oscillator's waveformColor.
- `left: 52; top: 8;` — name label, bold small (12/600), `textPrimary`. Text: oscillator name.
- `left: 52; top: 28;` — track label, caption (10/400), `textSecondary`. Text: track name.
- Right cluster (right-anchored inside row, 8px from right edge, each icon 24 × 24, 4px spacing):
  - `eye` / `eye-off` icon (visibility toggle).
  - `settings` icon (gear).
  - `trash-2` icon (delete).
  - Right cluster alpha: `0.4` when row NOT hovered/selected; `1.0` when hovered/selected.

For SELECTED row, beneath the above: processing-mode segmented bar from `shared-components`, positioned `left: 52; top: 56; width: 240; height: 28;` with `Stereo` segment active. 12px below it ends the row.

## Variant Definitions

### Variant 1 — DEFAULT

Label above frame: `DEFAULT (NOT HOVERED, NOT SELECTED)`.
- Row bg: transparent (sidebar bg shows through).
- Name: `Lead Vocal`, track `Track 1`, color dot `#1FD4F3`.
- Eye icon: `eye` (open).
- Right icons alpha 0.4.
- No processing-mode bar.

### Variant 2 — HOVERED

Label: `HOVERED`.
- Row bg: `rgba(232,234,237,0.05)`.
- Name `Lead Vocal`, track `Track 1`, color dot `#1FD4F3`.
- Eye icon open.
- Right icons alpha 1.0.
- Slight left indicator: 2px vertical bar on left edge `#1FD4F3 @ 0.0` (invisible; placeholder for hover).

### Variant 3 — SELECTED & EXPANDED

Label: `SELECTED & EXPANDED`.
- Row bg: `rgba(31,212,243,0.08)`.
- Left border-radius shows 2px vertical bar flush to left edge in `#1FD4F3`, height of row.
- Name `Lead Vocal` at `textHighlight`, track `Track 1` at `textSecondary`.
- Color dot `#1FD4F3`.
- Eye open.
- Right icons alpha 1.0.
- Processing-mode segmented bar visible below, `Stereo` active.

### Variant 4 — HIDDEN (VISIBILITY OFF)

Label: `HIDDEN (VISIBILITY TOGGLED OFF)`.
- Row bg transparent.
- Name `Room Mic` at `textMuted`, track `Track 6` at `textMuted`, color dot `#3AB0E8` at 0.4 alpha.
- Eye icon: `eye-off`, alpha 1.0.
- Other right icons alpha 0.4.
- Apply a full-row semi-transparent black overlay at `rgba(0,0,0,0.40)` on top of everything EXCEPT the eye-off icon (achieve by placing the overlay below the eye icon in stacking order — use `position: absolute; z-index: 1` for overlay, `z-index: 2` for eye icon; rest of icons under overlay).

### Variant 5 — UNASSIGNED PANE

Label: `UNASSIGNED PANE (AWAITING PANE PICK)`.
- Row bg `rgba(221,187,0,0.06)`.
- Left 2px vertical bar `#D4B02C` (warning tint) flush to left edge.
- Name `FX Return`, track `Aux 1`, color dot `#D4B02C`.
- Eye icon: `eye-off`, alpha 1.0 tinted `#D4B02C`.
- Small inline caption after track label, in same row: `Assign a pane to show` in 10px `#D4B02C`. Position: `left: 52; top: 42; width: 300; height: 14;`.
- Right icons alpha 1.0.

### Variant 6 — DRAGGING (GHOST + DROP INDICATOR)

Label: `DRAGGING (GHOST + DROP INDICATOR)`.
- Base row: `Bass DI`, `Track 3`, color dot `#4AD070`, visible at alpha 0.35 (ghost).
- Above base row, offset `top: 72` relative to frame, render another full row identical but at alpha 1.0 with a subtle `box-shadow: 0 8px 24px rgba(0,0,0,0.5);` — this is the floating drag preview.
- Beneath ghost, 2px horizontal line spanning full 488 width in `#1FD4F3` — this is the drop indicator.

## Negative Constraints (this file)

- Do NOT show more than one row per variant except variant 6.
- Do NOT include the full sidebar.
- Do NOT add tooltips.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] Exactly 6 frames.
- [ ] Variants 1, 2, 4, 5 frames 520 × 120; variant 3 frame 520 × 160; variant 6 frame 520 × 180.
- [ ] Variant 3 has selected-state left border and processing-mode bar.
- [ ] Variant 4 shows eye-off icon above the dim overlay.
- [ ] Variant 5 uses warning yellow `#D4B02C` accent.
- [ ] Variant 6 has both ghost row and floating preview plus blue drop indicator.
