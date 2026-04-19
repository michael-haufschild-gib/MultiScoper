# View: modal-theme-editor

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `modal-theme-editor.html` with 2 frames showing the Theme Editor modal.

## Frame

Full plugin canvas 1200 × 760 with dimmed backdrop `rgba(5,7,11,0.60)` and the `main-plugin-layouts` variant-1 shell as background.

## Modal Container

Wide modal: `left: 160; top: 90; width: 880; height: 580;`.
- bg `#161B24`, border 1px `rgba(232,234,237,0.18)`, radius 12.
- Shadow `0 8px 24px rgba(0,0,0,0.35)`.
- Internal padding 0 (panel handles its own padding).

## Structure — Two-Column Split

Inside the modal, two sub-panels separated by a 1px `#1F2630` vertical divider at x = 360.

### Left Panel — Preview (360 wide)

- Internal padding 24.
- Title at top: `Preview` 13/600 `textPrimary`.
- 16px gap.
- Below: 4 stacked mini waveform previews, each 312 × 96, radius 8, `#0F131A` background with grid overlay. Waveforms in `#1FD4F3`, `#E24BA6`, `#E87B3A`, `#4AD070` top to bottom. 8px gap between them.
- These preview tiles reflect the currently-selected theme's colors.

### Right Panel — Editor (520 wide)

- Internal padding 24.
- Header row 32 tall: title `Theme Editor` 15/600 left, X close icon 24 × 24 right.
- 16px gap.
- `Theme Name` label (10/600 caps letter-spacing 0.12em `textSecondary`) + text field 32 tall, content `Custom Dark`.
- 16px gap.
- `Colors` section — grid of 6 color-token rows:

Each color row is 32 tall with:
- Left: token name 13/400 `textPrimary` (120 wide).
- Middle: inline hex input 100 × 24, bg `#0F131A`, content like `#1A1A2E`.
- Right: 24 × 24 color swatch preview (rounded 4, filled with the hex value). Click-to-open in a real UI; here static.

Rows, in order:
1. `Background` → `#0A0D12`
2. `Pane` → `#0F131A`
3. `Grid` → `#2D3748`
4. `Text` → `#E2E8F6`
5. `Accent` → `#1FD4F3`
6. `Status Active` → `#00DD00`

- 16px gap.
- `Waveform Colors` label. Below: 8-swatch-wide, 2-row grid of swatches from the waveformColors palette. Swatches 28 × 28 radius 4, 6px gap. A `+` tile 28 × 28 dashed border at the end to add a custom color.
- 16px gap.
- Hex input pair row (2 fields side by side): `#00D9FF` and `#E2E8F0`, each 180 × 28.
- 16px gap.
- Footer row, bottom-pinned at 24 inside padding:
  - `Save Theme` Primary button 130 × 32 left.
  - `Cancel` Secondary button 100 × 32 right.

## Variant 1 — DEFAULT (editor open, no picker)

Label: `THEME EDITOR — DEFAULT`.

Render the layout exactly as above. No floating color picker.

## Variant 2 — COLOR PICKER OPEN

Label: `THEME EDITOR — COLOR PICKER OPEN`.

Same as variant 1 but with the full color-picker popup floating over the Accent row. The picker originates at the Accent swatch (small). Popup:
- 280 × 320, bg `#161B24`, border 1px `#242A35`, radius 10, shadow `0 8px 24px rgba(0,0,0,0.35)`.
- Position: `left: [accent_swatch_x - 256]; top: [accent_swatch_y + 32];` (flush right to swatch).
- Internals:
  - 16px padding.
  - SV rectangle 248 × 140 gradient for hue 188°, marker circle 14 × 14 white border at (80%, 90%).
  - 12px gap.
  - Hue slider 248 × 14 linear-gradient spectrum, marker bar 3 × 20 at 188° position (near teal).
  - 8px gap.
  - Alpha slider 248 × 14 checkerboard + gradient.
  - 12px gap.
  - Hex row: two fields 118 × 24 side by side, `#1FD4F3` and `rgb(31,212,243)`.
  - 12px gap.
  - Waveform color swatch strip 8 × 2 (16 swatches of waveformColors) 20 × 20 each.
  - 12px gap.
  - `Apply` Primary button 80 × 28 right-aligned.

Additionally: in variant 2, show a 2px border `#1FD4F3` around the Accent row's swatch to indicate it's being edited.

## Negative Constraints (this file)

- Do NOT include theme import/export buttons — skipped in this view.
- Do NOT show theme list. This mockup focuses on single-theme editing.
- Do NOT add contrast-ratio warning indicators here (future feature).

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] 2 frames at 1200 × 760 each.
- [ ] Modal 880 × 580 centered horizontally.
- [ ] Left panel shows 4 mini preview tiles with different colored waveforms.
- [ ] Color rows include exactly 6 named tokens in the order listed.
- [ ] Variant 2 shows floating color picker with SV rectangle, hue slider, alpha slider, hex fields, swatch strip, and Apply button.
- [ ] Variant 2 Accent row swatch has an edit indicator border.
