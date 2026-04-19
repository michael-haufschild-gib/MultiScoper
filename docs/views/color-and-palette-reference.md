# View: color-and-palette-reference

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `color-and-palette-reference.html` — a single sheet showing every theme token, every default waveform color, and every accent with its exact hex value. This is a reference for OTHER view prompts to cross-check against.

## Frame Override

`width: 1200px; height: 1200px; background: #0F131A; position: relative; padding: 40px;`.

## Layout

Four column groups, each group anchored at absolute x positions.

Group 1 — `BACKGROUNDS & SURFACES` (left: 40; top: 40; width: 260):
- Section title `BACKGROUNDS & SURFACES` 12/600 caps `textSecondary` letter-spacing 0.12em.
- 1px `#1F2630` divider below title at 24 below.
- Token rows starting at `top: 56`, each row 56 tall:
  - Left: 40 × 40 swatch radius 6 filled with token color.
  - Token name 12/600 `textPrimary` right of swatch, 12px gap.
  - Hex value 11/400 tabular-nums `textSecondary` below the name.
- Tokens (in order):
  - `backgroundPrimary` `#0A0D12`
  - `backgroundSecondary` `#10141B`
  - `backgroundPane` `#0F131A`
  - `backgroundRaised` `#161B24`
  - `controlBackground` `#161B24`
  - `controlBorder` `#242A35`
  - `controlHighlight` `#1F2630`
  - `divider` `#1F2630`

Group 2 — `TEXT & SIGNAL` (left: 340; top: 40; width: 260):
- Same structure.
- Tokens:
  - `textPrimary` `#E8EAED`
  - `textSecondary` `#8A94A3`
  - `textHighlight` `#FFFFFF`
  - `textMuted` `#4B5563`
  - `gridMajor` `#242A35`
  - `gridMinor` `#171C25`
  - `gridZeroLine` `#3B4555`
  - `crosshairLine` `#FFFFFF @ 0.60`

Group 3 — `STATUS & CONTROL` (left: 640; top: 40; width: 260):
- Same structure.
- Tokens:
  - `controlActive` `#1FD4F3`
  - `statusActive` `#00DD00`
  - `statusWarning` `#DDBB00`
  - `statusError` `#EE4444`
  - `btnPrimaryBg` `#007ACC`
  - `btnPrimaryBgHover` `#008AD9`
  - `btnPrimaryBgActive` `#0062A3`
  - `btnSecondaryBg` `#3A3A3A`

Group 4 — `ACCENTS` (left: 940; top: 40; width: 220):
- Same structure.
- Tokens:
  - `accentMagenta` `#E24BA6`
  - `accentOrange` `#E87B3A`
  - `accentYellow` `#D4B02C`
  - `accentGreen` `#4AD070`
  - `accentViolet` `#9F70E5`
  - `accentRed` `#E05A4A`

## Waveform Colors Section (below all four groups)

At `top: 580; left: 40; right: 40;`:
- Section title `DEFAULT WAVEFORM COLORS (index 0 → 15)` 12/600 caps `textSecondary`.
- 1px divider.
- 16 swatches in a single row, each 60 × 60 radius 8, 8px gap.
  - Index label ABOVE swatch: `0` `1` … `15` (10/600 tabular-nums).
  - Hex value BELOW swatch: the exact hex.
- Colors in this exact order:
  - `0 #1FD4F3`, `1 #E24BA6`, `2 #4AD070`, `3 #E87B3A`, `4 #9F70E5`, `5 #E05A4A`, `6 #D4B02C`, `7 #3AB0E8`, `8 #C55AC5`, `9 #6EC970`, `10 #F09050`, `11 #7070E5`, `12 #E06060`, `13 #E0C040`, `14 #50C5D0`, `15 #D05090`.

## Gradient Strips Section

At `top: 780; left: 40; right: 40;`:
- Section title `GRADIENTS (visual reference)`.
- Strip 1: meter-bar gradient `0-60% #4AD070 → 80% #D4B02C → 100% #EE4444`, 1120 × 24 radius 12.
- Strip 2: waveform stroke spectrum (16 waveformColors in order, each taking 1/16th width, 1120 × 24 radius 12).

## Border Alpha Samples

At `top: 880; left: 40; right: 40;`:
- Section title `BORDER ALPHA LEVELS (textPrimary over backgroundPane)`.
- 3 sample cards 200 × 100 radius 8 side-by-side:
  - `subtle (0.12)` — border `rgba(232,234,237,0.12)`.
  - `default (0.18)` — border `rgba(232,234,237,0.18)`.
  - `strong (0.30)` — border `rgba(232,234,237,0.30)`.
- Card bg `#0F131A`, token label centered in each.

## Shadow Samples

At `top: 1020; left: 40; right: 40;`:
- Section title `SHADOW — MODAL SURFACE ONLY`.
- Single sample card 240 × 80 radius 10 bg `#161B24` centered at left: 40. box-shadow `0 8px 24px rgba(0,0,0,0.35)`. Label `Modal surface shadow`.

## Negative Constraints (this file)

- Do NOT include colors that aren't in the theme.
- Do NOT adjust alpha on swatches — show full opacity (alpha effects belong to border samples).
- Do NOT add interactive copy-to-clipboard affordances.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] 4 column groups at the top, each rendering its listed tokens in order.
- [ ] 16 waveform swatches present in exact order, indexed 0..15.
- [ ] Gradient strip 1 transitions at 60% and 80%.
- [ ] Border alpha section has 3 cards at 0.12 / 0.18 / 0.30.
- [ ] Shadow sample uses exact `0 8px 24px rgba(0,0,0,0.35)`.
