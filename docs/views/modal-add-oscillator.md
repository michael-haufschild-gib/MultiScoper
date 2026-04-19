# View: modal-add-oscillator

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `modal-add-oscillator.html` with 3 frames stacked vertically, each showing the Add Oscillator modal in a different open state.

## Frame

Full plugin canvas 1200 × 760. Backdrop behind modal: `rgba(5,7,11,0.60)` filling entire frame. Main plugin (sidebar, panes) remains visible but dimmed — reuse the structure from `main-plugin-layouts` variant 1 as the backdrop.

## Modal Container

Centered horizontally and vertically: `left: 396; top: 140; width: 408; height: 480;`.

- Background `#161B24`.
- Border 1px `rgba(232,234,237,0.18)`.
- Border radius 12.
- Box shadow `0 8px 24px rgba(0,0,0,0.35)` (modal exception allowed).
- Internal padding 24.

### Modal Header

- `top: 24 (inside padding); height: 28;`
- Title `Add Oscillator` left, 15/600 `textPrimary`.
- X icon button 24 × 24 right.
- Below header: 1px `#1F2630` divider spanning full modal inner width, `top: 60;`.

### Modal Body (form fields)

Start at `top: 76;`. Each form field has:

- Caption label 10/400 caps letter-spacing 0.08em `textSecondary`, 14 tall.
- 4px gap.
- Control 32 tall.
- 16px gap after control.

Fields in order:

1. `SOURCE` — dropdown placeholder or value depending on variant.
2. `PANE` — pane selector component. See below.
3. `NAME` — text field, content depends on variant.
4. `COLOR` — color swatch strip: 16 swatches 20 × 20 radius 4 side-by-side (or wrapped 8 × 2). One swatch has 2px `#FFFFFF` border indicating selection.
5. `VISUAL PRESET` — dropdown, default `Default`.
6. Error label area (empty in default variant).

### Pane Selector Component Row

Pane selector, width full, 32 tall:
- Left: segmented bar of up to N pane tiles + an `+ New Pane` tile at the end. Each tile 64 × 32 radius 6.
- Existing pane tile: bg `#161B24`, border `#242A35`, text `Pane 1` 12/400 `textPrimary`.
- Active pane tile: bg `rgba(31,212,243,0.15)`, border `#1FD4F3`, text `#1FD4F3`.
- `+ New Pane` tile: bg `#161B24`, dashed border `#8A94A3 @ 0.4`, text `textSecondary`, plus icon 12 × 12 prefix.

### Modal Footer

Fixed at `bottom: 24 (inside padding); height: 36;`:
- Cancel button left, Secondary variant, width 180.
- OK button right, Primary variant, width 180 — label `Add Oscillator`.
- 8px gap between them.

## Variant 1 — DEFAULT (all closed)

Label: `DEFAULT — NOTHING SELECTED`.
- SOURCE dropdown: placeholder `Select source…` in `textMuted`.
- PANE: `Pane 1` active.
- NAME: placeholder `Oscillator name` in `textMuted`.
- COLOR: 5th swatch selected (color `#9F70E5`).
- VISUAL PRESET: `Default`.
- Error label: hidden.

## Variant 2 — SOURCE DROPDOWN OPEN

Label: `SOURCE DROPDOWN OPEN`.
- SOURCE dropdown shown CLOSED at the normal position, but popup floats below:
  - Popup 360 × 296 at `left: 24; top: [source_dropdown_bottom + 4];` (inside modal). Popup relative to modal inner.
  - Background `#161B24`, border `#242A35`, radius 6, shadow `0 8px 24px rgba(0,0,0,0.35)`.
  - Top: search field 336 × 28 inside the popup, with search icon 14 × 14 left, placeholder `Filter sources…`.
  - Section label `AVAILABLE SOURCES` 10/600 caps letter-spacing 0.12em `textSecondary`, 16 tall.
  - 7 source rows, each 32 tall with 12 padding:
    - Left: mic icon 14 × 14.
    - Name `Track 1 - Lead Vocal`, `Track 2 - Drums`, `Track 3 - Bass DI`, `Track 4 - Synth Pad`, `Track 5 - Acoustic Guitar`, `Master Bus`, `No Source (Disconnect)`.
    - Right: `stereo`/`mono` pill 44 × 16 radius 999 bg `rgba(31,212,243,0.15)` text `#1FD4F3` font 9/600.
    - Right-right: status dot 8 × 8 filled `#4AD070` (online).
    - First row highlighted: bg `rgba(31,212,243,0.15)`.
  - Row heights: 32 each. Total popup height auto-fits; cap at 296.
- All other modal fields unchanged from variant 1.

## Variant 3 — ERROR VALIDATION

Label: `ERROR — NO SOURCE SELECTED`.
- SOURCE dropdown empty, border 1px `statusError` (`#EE4444`).
- Error label directly beneath SOURCE dropdown (replacing the normal 16px gap):
  - 12/400 `statusError`, text `Please select a source.`
  - Leading 14 × 14 inline SVG `alert-triangle` outline in same color.
- PANE row unchanged.
- NAME empty, placeholder.
- COLOR: no selection (no swatch has border).
- Footer OK button: disabled Primary variant (bg `#353535`, text `#A0A0A0`).

## Negative Constraints (this file)

- Do NOT add tabs or sections in the modal — the modal is a flat form.
- Do NOT render the source-dropdown popup on variants 1 or 3.
- Do NOT add delete controls — this is ADD, not EDIT.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] 3 frames present, each 1200 × 760.
- [ ] Modal centered at 408 × 480.
- [ ] Backdrop dims underlying plugin.
- [ ] Variant 2 shows open source popup with 7 rows and search field.
- [ ] Variant 3 shows error label + red border on SOURCE.
- [ ] Footer OK button label reads `Add Oscillator` (not `OK`).
