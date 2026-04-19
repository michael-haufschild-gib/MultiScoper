# View: empty-state

READ `_README.md` first. Inherit all rules.

## Goal

Produce ONE HTML file `empty-state.html` showing the plugin when zero oscillators have been created. Single `.frame` at 1200 × 760. Label above frame: `EMPTY STATE — NO OSCILLATORS`.

Status: PROPOSED / NEW. Current production code renders a blank content area when `numPanes == 0`. This view defines the intended empty state.

## Shell

Same shell as `main-plugin-layouts` variant 1 (sidebar expanded 300, status bar 22).

## Content Viewport (left of sidebar)

Width 900, height 738. Background `#0A0D12`. Render a centered empty-state card:

- Card: 480 × 320, centered horizontally and vertically inside the content viewport (`left: 210; top: 209`).
- Background `#0F131A`, border 1px `rgba(232,234,237,0.12)`, radius 12.
- Inside the card, stacked absolutely:
  - Illustration area: `left: 0; top: 0; right: 0; height: 160;` — render an inline SVG 200 × 120 centered at top, showing 3 horizontal stacked oscilloscope lines in `#1FD4F3`, `#E24BA6`, `#4AD070` from top to bottom, each with the polyline from `_README`, semi-transparent (stroke opacity 0.4). Behind them, a faint grid overlay (4 major × 2 minor lines).
  - Title: centered, `top: 176; height: 24;` — text `No Oscillators Yet` at 18px weight 600 `textPrimary`.
  - Body copy: centered, `top: 208; left: 40; right: 40; height: 40;` — text `Add an oscillator to start visualizing any track in your session.` at 13px `textSecondary`, line-height 20, 2 lines max.
  - Primary CTA button: centered, `top: 260; width: 180; height: 36;` — `+ Add Oscillator`, Primary variant from `shared-components`. Plus icon 14 × 14 stroke 1.5 prefixed.

## Sidebar (expanded)

Render the sidebar EXACTLY as in `main-plugin-layouts` variant 1 EXCEPT the OSCILLATORS section content:

- OSCILLATORS header: count badge reads `0`, badge bg dimmed to `rgba(138,148,163,0.20)`, text `textMuted`.
- Below header: full-width Primary button `+ Add Oscillator`.
- Below button: empty-list placeholder — a 1px dashed `#242A35` rectangle 268 × 80 at 16px left/right padding, centered caption text `textMuted` 12px: `Oscillators will appear here`.

TIMING and OPTIONS sections render normally (identical to `main-plugin-layouts` variant 1), but with all controls in their default/initial state.

## Status Bar

Right cluster: `OpenGL | Src: 6 | Osc: 0 | Mem: 241.0 MB | CPU: 1.1% | 60.0 FPS`. FPS green.

Left zone hint: `Click Add Oscillator to begin` — 12px `textSecondary`.

## Negative Constraints (this file)

- Do NOT render any pane tiles in the content viewport.
- Do NOT animate the illustration.
- Do NOT use a modal — the CTA is inline in the card.

## Self-Verification Checklist

Inherit `_README` checklist. Additionally:
- [ ] Empty-state card is 480 × 320 and visually centered in content area.
- [ ] Sidebar OSCILLATORS count badge reads `0` in muted color.
- [ ] No pane tiles rendered.
- [ ] Sidebar still shows full TIMING and OPTIONS sections.
- [ ] Primary CTA uses exact `+ Add Oscillator` text.
