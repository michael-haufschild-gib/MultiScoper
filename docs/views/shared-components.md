# View: shared-components

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `shared-components.html` that renders every reusable control in every state. Downstream view prompts REFERENCE this file for visual parity — if the same button appears in `modal-add-oscillator` and `main-plugin-layouts`, it must look identical to the version here.

## Structure

Single frame is OK here — override the `_README` default size:

- Frame: `width: 1400px; height: 2200px; background: #0F131A; padding: 40px; position: relative; overflow: visible;`
- Inside, arrange sections as absolute-positioned columns at x = 40, 400, 760, 1120 (four columns, 340px wide each).
- Section headers: 12px uppercase `textSecondary`, letter-spacing 0.12em, with a 1px `divider` line 24px under the header label.
- Between controls within a section: 16px vertical gap (achieved by explicit `top:` values).

## Sections and Required States

Render EVERY state listed. A "state" is a separate rendered block with a 10px-uppercase label under it.

### Column 1 — Buttons

Each variant: 4 buttons side by side, each 72 × 32, labeled below: `default` / `hover` / `pressed` / `disabled`.

- `Primary` — bg `#007ACC` / hover `#008AD9` / pressed `#0062A3` / disabled `#353535` with text `#A0A0A0`. Text color `#FFFFFF` for non-disabled. Radius 6.
- `Secondary` — bg `#3A3A3A` / `#454545` / `#303030` / `#252525`. Text `#E0E0E0` → disabled `#606060`.
- `Danger` — bg `#EE4444` default, `#FF5555` hover, `#CC3030` pressed, `#353535` disabled. Text `#FFFFFF`.
- `Ghost` — transparent bg default, `rgba(255,255,255,0.10)` hover, `rgba(255,255,255,0.20)` pressed, transparent disabled. Text `#E0E0E0` / disabled `#606060`.
- `Icon` — 28 × 28 square, transparent bg default, `rgba(255,255,255,0.08)` hover, `rgba(255,255,255,0.16)` pressed. Inline SVG settings icon centered.

### Column 1 — Tabs + Segmented Bars

- Horizontal tabs: 4 tabs `Oscillators` / `Sources` / `Timing` / `Settings`. Active tab `#1FD4F3` underline 2px and text `textHighlight`. Inactive text `textSecondary`. Height 32.
- Vertical tabs: same 4 labels stacked, 120 × 32 each. Active tab bg `#161B24` with 2px left border in `controlActive`.
- Segmented (processing mode): 6 segments `Stereo|Mono|Mid|Side|L|R`, each 40 × 28, rounded container radius 6, active segment bg `rgba(31,212,243,0.15)` and text `#1FD4F3`. Inactive text `textSecondary`. 1px divider between segments.

### Column 2 — Input Fields

Width 280, height 32, radius 6, bg `controlBackground`, border 1px `controlBorder`.

- `empty` — placeholder `Enter value…` in `textMuted`.
- `focused` — border `controlActive`, 2px, with content `PF8FAFC`.
- `filled` — border `controlBorder`, content `PF8FAFC`.
- `error` — border `statusError`, inline error label below at 11px `statusError` with text `Invalid entry`.
- `disabled` — bg `rgba(22,27,36,0.5)`, border `controlBorder`, content `textMuted`, prefix search icon.
- `number input` — 280 × 32, embedded `-` button left (24×32) and `+` button right, center value `127`.

### Column 2 — Checkboxes and Radios

Checkboxes 18 × 18, radius 4, border 1px `controlBorder`:
- `empty`, `checked` (bg `#1FD4F3`, white check), `indeterminate` (bg `#1FD4F3`, white horizontal bar), `disabled` (bg `#252525`, border `controlBorder`).
- Labeled rows: `Empty`, `Checked group`, `Checked`, `Hover`, `Indeterminate`, `Disabled`.

Radios 18 × 18, radius 999:
- `empty`, `checked` (inner dot 8×8 `#1FD4F3`), `unselected`, `selected`, `indeterminate` (horizontal bar), `disabled`.

### Column 2 — Toggle Switches

Track 34 × 18, radius 999, thumb 14 × 14:
- `off` — track `#3A3A3A`, thumb left, `#E0E0E0`.
- `on` — track `#4AD070`, thumb right, `#FFFFFF`.
- `disabled on` — track `#3A3A3A` 50% alpha, thumb `#606060`.
- `on dim` — track `#1FD4F3`, thumb right.
- Labeled rows: `Sync to Host`, `Auto-Pause`.

### Column 3 — Sliders

Track full-width of the column (340), height 4, radius 2, filled portion `#1FD4F3`, unfilled `#242A35`. Thumb 14 × 14 circle `#FFFFFF`.

- `default` — label `Line Width` left, value `2.0px` right, slider below.
- `dual-handle range` — `Band` label, two thumbs, active range between thumbs colored `#1FD4F3`. Values `80 kHz` and `15 - 38`.
- `disabled` — track dim alpha 0.4, thumb `#8A94A3`.

### Column 3 — Badges

- Status pills 60 × 18 radius 999: `SYNCED` bg `rgba(0,221,0,0.20)` text `#4AD070`, `PAUSED` bg `rgba(221,187,0,0.20)` text `#D4B02C`, `ERROR` bg `rgba(238,68,68,0.20)` text `#EE4444`.
- Processing-mode pills 50 × 18: `STEREO`, `MONO`, `Mid`, `Side`, `L`, `R` — colored per the waveformColors list (first = `#1FD4F3`, etc.), text via contrast-safe white or `#0A0D12`.

### Column 3 — Dropdowns

Width 280, height 32, bg `controlBackground`, border `controlBorder`, radius 6, chevron-down SVG right 10.

- `closed` — label inside `Source: Track 1 - Vocals`, text `textPrimary`.
- `open` — popup 280 × 160 below with 3 items: `Hover` (bg `#1F2630`), `Selected item` (bg `rgba(31,212,243,0.15)` + check SVG left), `Selected item` (plain). Item height 36, padding 12, separator `#1F2630` 1px between.
- `multi-select` — selected chips inline: `Selected item 1 [x]`, `Selected item 7 [x]`. Chip 120 × 22 radius 999 bg `rgba(31,212,243,0.15)` text `#1FD4F3`, x-icon 12×12.

### Column 4 — Accordion Sections

Width 320, section header 40 × full-width bg `backgroundPane`:
- `collapsed` — chevron-right left, text `Advanced Settings`, count badge right `8` (18 × 18 circle bg `rgba(31,212,243,0.20)`).
- `expanded` — chevron-down left, text `Visual Options`, plus-icon right.

Nested items inside expanded: 4 rows labeled `Nested Item 1…4`, each 36 tall with a right-aligned control (dropdown or text value).

### Column 4 — Color Picker

Vertical panel 320 × 420, bg `backgroundRaised`, radius 10, padding 16:
- Title row: `COLOR PICKER` uppercase smallcaps label.
- Swatch grid 8 × 2 rows, each swatch 24 × 24 radius 6 — use the 16 waveformColors in order.
- HSV rectangle 288 × 160 gradient (saturation horizontal, value vertical for `hue 188°`). Marker circle 14 × 14 white border at approx 80% sat, 90% value.
- Hue slider 288 × 14 linear gradient rainbow below.
- Alpha slider 288 × 14 checkerboard + linear gradient.
- Hex inputs row: two 130 × 28 fields `#00D9FF` and `#E2E8F0`.
- `Apply` button bottom-right 80 × 32 Primary.

### Column 4 — Popup/Modal Recap

Reduced 280 × 200 mini-modal example:
- Title bar `Configure Oscillator` with X button right.
- Body: label `Form`, two radio options `Formation` / `Formation`, bottom row `Cancel` / `Save` buttons (70 × 28 each, right-aligned 8px gap).

### Column 4 — Meter Bar

Horizontal meter 320 × 10 radius 5, gradient left-to-right: `#4AD070` (0-60%), `#D4B02C` (60-80%), `#EE4444` (80-100%). Peak marker: 2px `#FFFFFF` vertical line at 78%.

### Column 4 — BPM / Transport

- Numeric display `120.0` in 32px bold with label `BPM` small above.
- Transport row: play / stop / pill-label `LINKED` with chain-link icon. Each icon button 24 × 24.

## Negative Constraints (this file)

- Do NOT add extra components not listed above.
- Do NOT restyle components arbitrarily. Use pinned tokens only.
- Do NOT label any control with "example" or "sample" text.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] All 5 button variants × 4 states = 20 buttons rendered.
- [ ] All 6 processing mode segments rendered in segmented bar.
- [ ] Dropdown "open" variant shows popup below with 3 items.
- [ ] Color picker shows exactly 16 swatches matching waveformColors.
- [ ] Meter bar gradient transitions at 60% and 80%.
