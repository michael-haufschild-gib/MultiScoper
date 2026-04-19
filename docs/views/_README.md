# UI Agent Contract — MultiScoper Plugin Mockups

YOU are a UI-generation LLM producing static HTML mockups of the MultiScoper audio plugin. READ this file IN FULL before any view file. Every view file references this contract.

## Output Contract

- Produce exactly ONE `*.html` file per view prompt.
- File is self-contained: inline `<style>`, inline SVG. No external CSS/JS/fonts/images.
- No scripts. No event handlers. No animations. No CSS transitions. No `:hover` rules. States are rendered side-by-side as separate DOM blocks.
- No comments in the HTML output.
- No `lorem ipsum`. Use the sample content section below verbatim.

## Canvas

- Each view renders inside a `<div class="frame">` of fixed size `width: 1200px; height: 760px;` with `position: relative; overflow: hidden;`.
- If a view prompt asks for N variants, stack N frames vertically with a 24px gap and a 12px label above each (label text supplied by prompt).
- Body background `#05070B`. Frame background per theme below.

## Layout Rules — HARD

- NEVER use `display: flex`. NEVER use `display: grid`. NEVER use `flex-*`, `grid-*`, `gap`, `place-*`.
- Use `position: absolute` with explicit `left/top/width/height` in pixels. Containers use `position: relative`.
- For inline rows of text+icon, use `display: inline-block` with explicit pixel widths and `vertical-align: middle`.
- No `transform`, no `filter` except where explicitly required (shadows). No `backdrop-filter`. No `calc()`.
- Units: `px` only. No `rem`, `em`, `%`, `vw`, `vh`.

## Typography

- `font-family: 'Inter', system-ui, -apple-system, 'Segoe UI', sans-serif;`
- Sizes:
  - `caption` 10px / 14px line-height / weight 400
  - `small` 12px / 16px / 400
  - `smallBold` 12px / 16px / 600
  - `body` 13px / 18px / 400
  - `bodyBold` 13px / 18px / 600
  - `title` 15px / 20px / 600
- `letter-spacing: 0.02em` on all-caps labels (`SIDEBAR SECTION HEADERS`). Otherwise 0.
- Numeric/metric values: `font-variant-numeric: tabular-nums;`

## Color Palette (dark theme — pinned)

Theme tokens — use these exact hex values:

| Token | Hex |
|---|---|
| `backgroundPrimary` | `#0A0D12` |
| `backgroundSecondary` | `#10141B` |
| `backgroundPane` | `#0F131A` |
| `backgroundRaised` | `#161B24` |
| `gridMajor` | `#242A35` |
| `gridMinor` | `#171C25` |
| `gridZeroLine` | `#3B4555` |
| `textPrimary` | `#E8EAED` |
| `textSecondary` | `#8A94A3` |
| `textHighlight` | `#FFFFFF` |
| `textMuted` | `#4B5563` |
| `divider` | `#1F2630` |
| `controlBackground` | `#161B24` |
| `controlBorder` | `#242A35` |
| `controlHighlight` | `#1F2630` |
| `controlActive` | `#1FD4F3` |
| `statusActive` | `#00DD00` |
| `statusWarning` | `#DDBB00` |
| `statusError` | `#EE4444` |
| `btnPrimaryBg` | `#007ACC` |
| `btnPrimaryBgHover` | `#008AD9` |
| `btnPrimaryBgActive` | `#0062A3` |
| `btnSecondaryBg` | `#3A3A3A` |
| `btnSecondaryBgHover` | `#454545` |
| `accentMagenta` | `#E24BA6` |
| `accentOrange` | `#E87B3A` |
| `accentYellow` | `#D4B02C` |
| `accentGreen` | `#4AD070` |
| `accentViolet` | `#9F70E5` |
| `accentRed` | `#E05A4A` |

Default per-oscillator waveform colors (use for color dots, waveform strokes, processing-mode badges):

`#1FD4F3`, `#E24BA6`, `#4AD070`, `#E87B3A`, `#9F70E5`, `#E05A4A`, `#D4B02C`, `#3AB0E8`, `#C55AC5`, `#6EC970`, `#F09050`, `#7070E5`, `#E06060`, `#E0C040`, `#50C5D0`, `#D05090`

Border alpha overlays (over `backgroundPane`): `subtle` = `textPrimary @ 0.12`, `default` = `@ 0.18`, `strong` = `@ 0.30`. Implement as `rgba(232,234,237,0.12)` etc.

## Spacing

- Outer frame padding: 0 (full-bleed). Sidebar has its own internal padding.
- Section padding in sidebar: 16px.
- Spacing scale: `XS 4`, `SM 8`, `MD 12`, `LG 16`, `XL 24`.
- Control heights: `32` (inputs/dropdowns), `36` (buttons), `28` (segmented), `24` (rows), `40` (accordion header), `40` (slider row).
- Corner radius: `panels 10px`, `controls 6px`, `pills 999px`, `color dots 999px`.
- Hairline divider: 1px `divider` token. NEVER use box-shadow for separators.

## Shells — Pinned Dimensions

- Main window: 1200 × 760.
- Right sidebar expanded: width 300. Collapsed rail: width 40.
- Status bar: height 22, full-width at bottom, `backgroundSecondary` fill.
- Pane corner radius: 10px.
- Pane header: height 32, tinted `rgba(232,234,237,0.08)` on pane bg.

## Icons — Inline SVG, Lucide Outline Style

- All icons: inline `<svg viewBox="0 0 24 24" width="W" height="W" fill="none" stroke="currentColor" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">`.
- Default W = 14 for row icons, 16 for button icons, 20 for section headers.
- Use these Lucide path shapes verbatim (copy from lucide.dev if needed):
  - `trash-2`, `settings`, `eye`, `eye-off`, `chevron-left`, `chevron-right`, `chevron-down`, `chevron-up`, `grip-vertical`, `x`, `plus`, `search`, `pause`, `play`, `square`, `link`, `bar-chart-3`, `lock`, `unlock`, `music`, `mic`, `sliders-horizontal`.
- NEVER use icon fonts, emoji-as-icons, or image files.

## Waveform Placeholder SVG

Where a view shows a rendered waveform inside a pane, use this exact polyline structure — a stereo scope line with a visible peak cluster. Color from the oscillator's assigned waveformColor:

```html
<svg width="100%" height="100%" viewBox="0 0 1000 200" preserveAspectRatio="none">
  <polyline fill="none" stroke="COLOR" stroke-width="1.5" stroke-linejoin="round"
    points="0,100 30,98 60,102 90,95 120,108 150,88 180,112 210,80 240,120 270,70 300,130 330,60 360,140 390,50 420,145 450,55 480,135 510,65 540,125 570,75 600,115 630,85 660,105 690,95 720,100 750,98 780,102 810,99 840,100 870,100 900,101 930,99 960,100 990,100 1000,100" />
</svg>
```

For multi-channel displays, stack two polylines offset ±40px on Y.

## Grid Overlay inside Panes

Behind the waveform, render a grid: 8 vertical × 4 horizontal lines, `stroke="#242A35" stroke-width="1"`, plus one zero line at vertical center `stroke="#3B4555" stroke-width="1"`.

## Sample Content — Use Verbatim

Track/source names (in order): `Lead Vocal`, `Drums - Kick`, `Bass DI`, `Acoustic Gtr`, `Synth Pad L`, `Synth Pad R`, `Room Mic`, `FX Return`.

Oscillator name defaults mirror track names.

Pane names: `Pane 1`, `Pane 2`, `Pane 3`, `Pane 4`.

Numeric samples: `BPM 120.0`, `FPS 60.0`, `CPU 4.2%`, `Mem 287.0 MB`, `Osc: 3`, `Src: 6`, `Gain 0.0 dB`, `Line Width 2.0 px`, `Opacity 100%`, `Time Interval 500.0 ms`.

Visual preset options (dropdown content, in this order): `Default`, `Neon Bloom`, `Phosphor CRT`, `Gradient Fill`, `Glass Refraction`, `Volumetric Ribbon`, `Particle Stream`, `Heat Signature`, `Wireframe Mesh`, `Liquid Chrome`, `Aurora Borealis`, `Digital Glitch`, `Dual Outline Glow`.

Processing mode labels: `Stereo`, `Mono`, `Mid`, `Side`, `L`, `R`. Icon glyphs: stereo = two stacked horizontal waves, mono = single wave, mid = plus inside circle, side = minus inside circle, L = letter L, R = letter R. Use minimal inline SVG.

Note interval labels: `1/32`, `1/16`, `1/12`, `1/8`, `1/4`, `1/2`, `1 Bar`, `2 Bars`, `3 Bars`, `4 Bars`, `8 Bars`, `1/8.`, `1/4.`, `1/2.`, `1/8T`, `1/4T`, `1/2T`.

Theme names: `Dark Professional`, `Light Modern`, `High Contrast`, `Classic Green`.

## Negative Constraints (hard no)

- No placeholder text like `…`, `TODO`, `lorem ipsum`.
- No external font links (`<link href="fonts.googleapis.com…">`).
- No `@font-face`. No web fonts.
- No `cursor:` declarations.
- No `transition`, `animation`, `@keyframes`.
- No `box-shadow` except a single `0 8px 24px rgba(0,0,0,0.35)` for modal containers only.
- No `backdrop-filter`, `filter`, `mix-blend-mode`.
- No emoji anywhere.
- No `aria-*` attributes (not the LLM's job).
- No `title` attributes.
- No inline `data-*` except `data-state="…"` when a prompt requests it for variant labeling.

## File Header Template (each view HTML must start with this)

```html
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<title>[view name]</title>
<style>
body { margin:0; padding:24px; background:#05070B; font-family:'Inter',system-ui,-apple-system,'Segoe UI',sans-serif; color:#E8EAED; font-variant-numeric: tabular-nums; }
.variant-label { font-size:12px; color:#8A94A3; text-transform:uppercase; letter-spacing:0.08em; margin:0 0 8px 0; }
.frame { position:relative; width:1200px; height:760px; overflow:hidden; margin:0 0 24px 0; background:#0A0D12; border-radius:10px; }
</style>
</head>
<body>
[variants]
</body>
</html>
```

## Self-Verification Checklist (run against your own output before returning)

Every view HTML must satisfy ALL of:

- [ ] No `display:flex` or `display:grid` anywhere in `<style>` or inline `style=`.
- [ ] No `transition`, `animation`, or `@keyframes`.
- [ ] No external URL in `<link>`, `<img>`, `<script>`, `@import`, `url(…)`.
- [ ] Every frame is exactly 1200 × 760.
- [ ] Font family includes `'Inter', system-ui`.
- [ ] All colors referenced are from the pinned palette, with two explicitly allowed exceptions: the page background `#05070B` (template-fixed) and the modal-container shadow `rgba(0,0,0,0.35)` (negative-constraint allowance).
- [ ] All numeric sample values match the sample content section verbatim.
- [ ] Every icon is inline SVG with `stroke="currentColor"` and `stroke-width="1.5"`.
- [ ] No comments, no emoji, no `aria-*`.
- [ ] If the prompt asked for N variants, the HTML contains exactly N `.frame` blocks plus N labels.
