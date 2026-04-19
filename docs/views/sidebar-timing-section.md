# View: sidebar-timing-section

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `sidebar-timing-section.html` with 3 side-by-side frames showing the TIMING accordion section in its three meaningful states.

## Frame Override

Each frame: `width: 320px; height: 420px; background: #10141B; padding: 16px; position: relative; display: inline-block; vertical-align: top; margin-right: 24px;` — three frames on one row.

## Common Section Chrome

Inside each frame:

- Accordion header row, `top: 0; left: 0; right: 0; height: 40;`:
  - Left: chevron-down icon 14 × 14 at `left: 8; top: 13`.
  - Section label `TIMING` in uppercase 12/600 `textSecondary`, letter-spacing 0.12em, `left: 32; top: 13`.
- Content area starts at `top: 48` with 16px left/right internal padding.
- Bottom hairline: 1px `#1F2630` at `bottom: 0`.

## Variant 1 — TIME MODE

Frame label: `TIME MODE`.

- Segmented bar (mode toggle), `top: 48; left: 16; width: 288; height: 28;`:
  - Two buttons `TIME` (active, `#1FD4F3` bg tint at 0.15 text `#1FD4F3`) / `MELODIC` (inactive text `textSecondary`).
- 16px spacing.
- Row: Label `Mode` (40 wide, 32 tall, 13/400 `textPrimary`) + Dropdown `Free Running` (filling remaining width, 32 tall).
- 16px spacing.
- Numeric text field full-width, 32 tall, content `500.0`, suffix `ms` right-aligned inside field, label above field reading `Interval` in 10px caps `textSecondary`.

### No BPM controls visible in TIME mode.

## Variant 2 — MELODIC MODE, FREE-RUNNING BPM

Frame label: `MELODIC — FREE BPM`.

- Segmented bar: `TIME` inactive / `MELODIC` active.
- 16px.
- Row: `Mode` + Dropdown `Free Running`.
- 16px.
- Note interval dropdown full-width, 32 tall, displaying `1/4` — chevron shows open potential but rendered CLOSED here.
- 16px.
- BPM row, 32 tall:
  - Label `BPM` (30 wide) left.
  - Numeric input field `120.0` (width = remaining - 70 - 16).
  - Toggle `Sync` (70 wide, toggle OFF so track `#3A3A3A`).
  - Layout: 30 / 8 / field / 8 / 70.

### No SYNCED pill.

## Variant 3 — MELODIC MODE, HOST-SYNCED

Frame label: `MELODIC — HOST SYNCED`.

- Segmented bar: `MELODIC` active.
- 16px.
- Row: `Mode` + Dropdown `Restart on Play`.
- 16px.
- Note interval dropdown OPEN — render the base dropdown closed appearance at the normal position showing current value `1/4`, but show the popup floating below with 7 items visible (of the 17 full list); scroll indicators on top+bottom:
  - Popup: `top: 184; left: 16; width: 288; height: 196; background: #161B24; border: 1px solid #242A35; border-radius: 6;` with `box-shadow: 0 8px 24px rgba(0,0,0,0.35)` (ALLOWED exception per `_README` modal rule — this popup counts as a modal surface).
  - Items 28 tall each, padding 12, content: `1/16th`, `1/8th`, `1/4` (bg `rgba(31,212,243,0.15)` text `#1FD4F3`), `1/2`, `1 Bar`, `2 Bars`, `4 Bars`.
- BPM row beneath the popup in the same position as variant 2 BUT:
  - `BPM` label + read-only value `120.0` with a lock icon 14 × 14 left of value (Lucide `lock`). Text color `textHighlight`.
  - Toggle `Sync` ON (track `#4AD070` thumb right white).
- Below BPM row, 12px gap, centered pill:
  - `SYNCED` badge, 70 × 18 radius 999, bg `rgba(0,221,0,0.20)` text `#4AD070`, font 10/600 letter-spacing 0.12em.

## Negative Constraints (this file)

- Do NOT show the full sidebar — only the TIMING section frame.
- Do NOT render other accordion sections.
- Do NOT animate dropdown open state — show it statically positioned.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] Exactly 3 frames, 320 × 420 each, inline-block side-by-side.
- [ ] Variant 1 has NO BPM row and NO note interval dropdown.
- [ ] Variant 2 has BPM input editable and NO SYNCED pill.
- [ ] Variant 3 shows SYNCED pill, note-interval popup open, and lock icon next to BPM value.
- [ ] The note interval popup in variant 3 has the currently selected row `1/4` tinted cyan.
