# View: modal-config-oscillator

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `modal-config-oscillator.html` with 3 frames showing the Oscillator Configuration modal.

## Frame

Full plugin canvas 1200 × 760 with dimmed backdrop `rgba(5,7,11,0.60)` and the `main-plugin-layouts` variant-1 shell as background.

## Modal Container

Centered: `left: 380; top: 60; width: 440; height: 640;`.
- Background `#161B24`, border 1px `rgba(232,234,237,0.18)`, radius 12.
- Box-shadow `0 8px 24px rgba(0,0,0,0.35)`.
- Internal padding 24.

### Modal Header

- Inline editable name field at top, 32 tall, width 320 left-aligned. Current value `Lead Vocal` 15/600 `textPrimary`. Cursor bar blink NOT rendered (static). Right of name (inside header row), a 16 × 16 color dot `#1FD4F3`. Right-most: X close icon 24 × 24.
- Below: 1px `#1F2630` divider.

### Modal Body (scrollable conceptually, but render all at once)

Form fields in this order, each with caption label (10/600 caps `textSecondary` letter-spacing 0.12em) + control:

1. `SOURCE` — source selector combo (see variant 2).
2. `PROCESSING MODE` — segmented bar 6 segments `Stereo|Mono|Mid|Side|L|R`, `Stereo` active.
3. `COLOR` — 16 swatches 20 × 20 radius 4, selected swatch has 2px white border (selected = `#1FD4F3`, the first).
4. `VISUAL PRESET` — dropdown `Default`. Tooltip indicator (small i-circle 12 × 12) right of the label `textSecondary`.
5. Slider row `Line Width` / value `2.0 px`.
6. Slider row `Opacity` / value `100%`.
7. `PANE` — pane selector bar (same style as `modal-add-oscillator`), with `Pane 1` active.

### Modal Footer

Fixed at bottom, 36 tall:
- Delete button (Danger variant) LEFT, 120 wide, label `Delete Oscillator`, icon `trash-2` 14 × 14 prefix.
- Close button (Secondary) RIGHT, 100 wide, label `Close`.

## Variant 1 — DEFAULT EDIT STATE

Label: `DEFAULT — EDITING LEAD VOCAL`.

All fields in their populated state as listed above. No open popups.

## Variant 2 — SOURCE DROPDOWN OPEN

Label: `SOURCE DROPDOWN OPEN`.

Identical to variant 1 except SOURCE field row shows its dropdown OPEN:
- Render the source selector closed at normal position showing current `Track 1 - Main Vocal`.
- Float popup 392 × 320 below the source field at `left: 24; top: [source_dropdown_bottom + 4]`. Same styling as `modal-add-oscillator` variant 2 source popup.
- Popup has search field and 7 rows as listed in `modal-add-oscillator.md`. First row currently selected.

## Variant 3 — DELETE CONFIRM ALERT

Label: `DELETE CONFIRM — DESTRUCTIVE CONFIRMATION`.

Base modal dims to `opacity: 0.4` (achieve visually by layering — do not use CSS filter; instead paint an additional semi-transparent layer over the modal at `rgba(10,13,18,0.50)` covering the modal rect). A smaller alert modal appears centered on screen:

- Alert container: `left: 460; top: 280; width: 280; height: 200;`.
- bg `#161B24`, border 1px `#EE4444 @ 0.4`, radius 12, shadow `0 8px 24px rgba(0,0,0,0.50)`.
- Inner padding 24.
- At top: alert triangle SVG 32 × 32 stroke `#EE4444`, centered horizontally.
- 12px gap, title `Delete Oscillator?` centered, 15/600 `textPrimary`.
- 8px gap, body `This action cannot be undone. The oscillator "Lead Vocal" will be removed.` 12/400 `textSecondary`, 3 lines max, centered.
- Bottom: two buttons 110 × 32 side by side, 8px gap, bottom-pinned at 24 inside padding:
  - `Cancel` Secondary (left).
  - `Delete` Danger (right).

## Negative Constraints (this file)

- Do NOT add advanced/visual-override sliders beyond Line Width and Opacity.
- Do NOT add vertical-scale/offset controls (historical design, not in production).
- Do NOT render the LFO list panel seen in some older mockups.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] 3 frames, each 1200 × 760.
- [ ] Name field at top is editable-looking, with color dot beside it.
- [ ] Variant 1 shows Line Width `2.0 px`, Opacity `100%`, Pane `Pane 1` active.
- [ ] Variant 2 has source popup open with 7 rows.
- [ ] Variant 3 shows smaller Delete-confirm alert layered over the dimmed config modal.
- [ ] Delete button in footer is ALWAYS visible on variants 1 and 2 and uses Danger variant.
