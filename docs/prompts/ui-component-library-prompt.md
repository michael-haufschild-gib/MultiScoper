# Oscil UI Component Library - Image Prompt

Optimized prompt for Google Nano Banana (Gemini Image Generation) to create a comprehensive UI component reference sheet for the Oscil audio plugin.

---

## Complete Component Library Prompt

A comprehensive UI component library reference sheet displays every interface element for a professional audio oscilloscope plugin against a deep charcoal (#1A1A2E) canvas. The layout presents components in organized sections with clear labels in muted slate (#94A3B8) uppercase text, demonstrating all interactive states from default through hover, active, focused, and disabled.

**Buttons Section:**
Four rows showcase button variants in the plugin's design language. Primary buttons in confident blue (#3B82F6) with white (#F8FAFC) text display states: default with subtle bottom shadow, hover with brightened background (#4F8FFF), pressed with inset shadow and darkened fill (#2563EB), and disabled at 40% opacity with no cursor interaction. Secondary buttons in dark slate (#1E293B) with light borders (#334155) show matching state progression. Danger buttons in error red (#EF4444) appear for destructive actions. Ghost buttons render as transparent with blue text only. Icon buttons display as 32px squares with centered glyphs. All buttons feature 8px border radius and 44px minimum touch height.

**Input Fields Section:**
Text inputs in dark charcoal (#242438) with rounded corners (6px) and subtle inset border (#334155) display across states: empty with placeholder text "Enter value..." in muted gray (#64748B), focused with blue (#3B82F6) border glow and white text cursor, filled with white (#F8FAFC) text content, error state with red (#EF4444) border and inline error message below, and disabled with reduced opacity background. A search variant shows magnifying glass icon prefix. Number inputs feature compact plus/minus stepper buttons (#334155 background) flanking the value display, with the value "127" centered in monospace font.

**Select Dropdowns:**
Closed select displays "Source: Track 1 - Vocals" with downward chevron, matching input field styling. Expanded state shows floating panel in slate (#1E293B) with 8px shadow, listing options with hover highlight (#334155), selected item marked with checkmark icon and blue (#3B82F6) background tint. Multi-select variant shows selected items as removable pill badges within the field.

**Tabs Component:**
Horizontal tab bar presents four tabs: "Oscillators" (active, blue #3B82F6 bottom border, white text), "Sources" (default, gray text), "Timing" (hover, subtle background), "Settings" (disabled, 40% opacity). Vertical tab variant appears below for sidebar navigation context with active indicator as left border.

**Toggle Switches:**
Compact 40x20px toggle switches display in row: off state with dark track (#334155) and gray knob, on state with green (#22C55E) track and white knob shifted right, disabled off with muted colors, disabled on maintaining green but reduced opacity. Labels "Sync to Host" and "Auto-Pause" accompany example switches.

**Checkboxes and Radio Buttons:**
Checkbox group shows: unchecked with empty rounded square (#334155 border), checked with blue (#3B82F6) fill and white checkmark, indeterminate with horizontal dash, hover with subtle glow, disabled states. Radio button group demonstrates: unselected with empty circle, selected with blue filled inner dot, disabled variants. Labels in white (#F8FAFC) align right of controls.

**Sliders:**
Horizontal sliders span 200px width showing: default with charcoal (#242438) track and blue (#3B82F6) fill to current position, thumb as 14px circle with subtle shadow, value tooltip "2.0px" floating above thumb on hover. Labeled variants show "Line Width" left-aligned and current value right-aligned. Vertical slider variant for mixing contexts. Dual-handle range slider shows selectable frequency band. Disabled slider appears with gray track and static thumb.

**Badges and Tags:**
Status badges display inline: "SYNCED" in green (#22C55E) pill with white text, "PAUSED" in amber (#F59E0B), "ERROR" in red (#EF4444), "STEREO" in blue (#3B82F6) outline style, "MONO" in muted (#64748B) outline. Processing mode tags show "Mid", "Side", "L", "R" as compact 24px pills.

**Color Picker:**
Compact color picker shows current color swatch (#00D9FF) as 24px square, clicking reveals expanded panel: saturation/brightness gradient square with crosshair selector, vertical hue strip rainbow slider, hex input field showing "#00D9FF", preset palette row of 16 waveform colors, opacity slider, and "Apply" button.

**Accordion Sections:**
Sidebar accordion demonstrates collapsed state with "▸ Advanced Settings" header and expand arrow, expanded state "▾ Visual Options" revealing nested controls with smooth 12px padding, section dividers as thin lines (#334155), nested content indented 16px.

**Sidebar Section Headers:**
Section organization showing "OSCILLATORS" header in small caps (#94A3B8) with count badge "8", followed by list content with 8px vertical spacing between items, optional collapse/expand toggle, and "Add Oscillator" action button aligned right of header.

**Popup/Modal Overlay:**
Floating popup panel in dark slate (#1E293B) demonstrates: title bar with "Configure Oscillator" and X close button, body content area with form controls, footer with "Cancel" secondary button and "Save" primary button, 12px border radius, drop shadow (0px 8px 32px rgba(0,0,0,0.5)), semi-transparent backdrop (#000000 at 50% opacity) visible around edges.

**Additional Plugin-Specific Components:**
Waveform color selector as horizontal row of 8 color swatches with active ring indicator. Processing mode segmented control with six equal segments "Full Stereo | Mono | Mid | Side | L | R", active segment highlighted blue (#3B82F6). Meter bar showing signal level with gradient fill green-to-amber-to-red. BPM display field showing "120.0" in tabular monospace with subtle beat pulse animation indicator. Transport sync badge showing play/stop icons with "LINKED" status.

Professional DAW plugin component library, JUCE framework visual language, 8px grid spacing system, consistent border radius (6px inputs, 8px buttons, 12px panels), organized specimen sheet layout, 4K resolution UI design reference, comprehensive dark mode design system, production-ready component states.

---

## Prompt Analysis

- **Subject**: Complete UI component library for VST audio plugin
- **Style**: Professional DAW plugin aesthetic matching JUCE framework, dark mode design system
- **Key Enhancements**: All interactive states (default, hover, active, focus, disabled, error), plugin-specific components (waveform color selector, processing mode segmented control, transport sync), exact hex color codes from existing design tokens
- **Quality Modifiers**: "Professional DAW plugin", "4K resolution", "JUCE framework visual language", "comprehensive dark mode design system", "production-ready component states"

---

## Design Tokens Reference

### Colors
| Token | Hex | Usage |
|-------|-----|-------|
| Background Primary | #1A1A2E | Main canvas |
| Background Secondary | #242438 | Sidebar, inputs |
| Background Tertiary | #1E293B | Dropdowns, popups |
| Accent Blue | #3B82F6 | Primary actions, active states |
| Accent Blue Hover | #4F8FFF | Button hover |
| Accent Blue Pressed | #2563EB | Button pressed |
| Success Green | #22C55E | Toggle on, success badges |
| Warning Amber | #F59E0B | Warning states |
| Error Red | #EF4444 | Error states, danger buttons |
| Text Primary | #F8FAFC | Main text |
| Text Secondary | #94A3B8 | Labels, placeholders |
| Text Muted | #64748B | Disabled text |
| Border | #334155 | Input borders, dividers |

### Spacing
- Grid: 8px base unit
- Small gap: 4px
- Standard padding: 16px
- Section spacing: 24px

### Border Radius
- Inputs: 6px
- Buttons: 8px
- Panels/Modals: 12px

### Component Sizing
- Minimum touch target: 44px
- Icon buttons: 32px square
- Toggle switches: 40x20px
- Color swatches: 24px square
- Slider thumb: 14px diameter
