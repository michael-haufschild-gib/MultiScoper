# Oscil Multi-Track Oscilloscope Plugin - UI Image Prompts

Optimized prompts for Google Nano Banana (Gemini Image Generation) to create detailed UI mockups for the Oscil audio plugin.

---

## 1. Main Interface - Dark Theme (Primary View)

A professional audio plugin interface presents a sophisticated multi-track oscilloscope visualization system against a deep charcoal (#1A1A2E) background. The main viewing area dominates the left side, displaying three stacked oscilloscope panes with crisp waveform traces in vibrant colors - electric cyan (#00D9FF), hot magenta (#FF006E), and lime green (#39FF14) - rendered against subtle grid lines in muted slate (#2D3748). Each pane features a thin title bar showing "Track 1 - Vocals", "Track 2 - Drums", "Track 3 - Bass" in clean sans-serif typography.

- Waveforms display smooth, continuous audio signals with varying amplitudes
- Subtle glow effects (#00D9FF with 20% opacity) around active waveforms
- Grid lines at 10% opacity creating depth without distraction
- Pane resize handles visible between sections as thin horizontal lines

The right sidebar panel in slightly lighter charcoal (#242438) contains organized control sections with rounded input fields, dropdown menus, and toggle switches. A prominent "Add Oscillator" button in accent blue (#3B82F6) sits at the top. Below, a scrollable oscillator list shows colored indicator dots matching their waveform colors. The bottom status bar displays "CPU: 4.2% | Memory: 287MB | 60 FPS | OpenGL" in small monospace text with a subtle green (#22C55E) status indicator.

Professional DAW plugin aesthetic, JUCE framework visual style, 4K resolution UI mockup, crisp anti-aliased edges, subtle shadows for depth, modern dark mode design system.

---

## 2. Main Interface - Light Theme

A clean professional audio oscilloscope plugin interface presents against a pristine off-white (#F8FAFC) background with subtle warm undertones. The main visualization area shows three oscilloscope panes with waveform traces in rich, saturated colors - deep ocean blue (#0066CC), coral red (#FF6B6B), and forest green (#2D9A2D) - contrasting beautifully against the light grid (#E2E8F0). Each pane has a white (#FFFFFF) header bar with hairline bottom borders in light gray (#CBD5E1).

- Waveforms render as crisp, precise lines with subtle anti-aliasing
- Grid lines provide measurement reference without visual clutter
- Soft drop shadows (0px 1px 3px rgba(0,0,0,0.1)) separate panes elegantly
- Time markers along the bottom axis in muted gray (#64748B) text

The right sidebar in pure white (#FFFFFF) features a subtle left border (#E2E8F0) separating it from the oscilloscope area. Control sections are organized with clear section headers in darker text (#1E293B). Input fields have light gray (#F1F5F9) backgrounds with rounded corners. The "Add Oscillator" button stands out in confident blue (#2563EB) with white text. A status bar at the bottom shows performance metrics in slate gray (#475569) text.

Professional audio software aesthetic, Apple-inspired clarity, JUCE framework styling, 4K resolution, precise typography, clean minimalist design system.

---

## 3. Two-Column Layout View

A professional audio oscilloscope interface displays a sophisticated two-column pane arrangement for multi-track analysis. The left column contains two oscilloscope panes stacked vertically showing vocals (electric cyan #00D9FF) and guitar (amber #F59E0B) waveforms. The right column displays drums (hot magenta #FF006E) and bass (lime green #39FF14) in equally sized panes. The dark charcoal (#1A1A2E) background unifies the layout.

- Column divider visible as a subtle 2px line in slate (#334155)
- Each pane maintains independent scroll and zoom controls
- Waveforms render with consistent line weight across all panes
- Pane headers show source names and processing mode indicators
- Equal vertical spacing distributes panes harmoniously

The collapsed sidebar on the right shows only a thin strip (#242438) with an expand arrow icon, maximizing the oscilloscope viewing area. The status bar spans the full width at bottom. Four distinct waveform colors create visual separation between audio sources.

Professional DAW plugin interface, grid-based layout system, 8px spacing rhythm, 4K resolution, balanced visual composition, modern production software aesthetic.

---

## 4. Three-Column Layout View

A comprehensive audio analysis interface presents oscilloscopes arranged in an efficient three-column grid layout. The leftmost column displays lead vocals (electric cyan #00D9FF) and backing vocals (sky blue #38BDF8). The center column shows drums (hot magenta #FF006E) and percussion (coral #FB7185). The right column contains bass (lime green #39FF14), synth pad (purple #A855F7), and guitar (amber #F59E0B). Deep charcoal (#1A1A2E) background provides consistent backdrop.

- Seven total panes distributed across three columns
- Column dividers rendered as 1px lines in dark slate (#1E293B)
- Varied pane heights accommodate different track importance
- Each waveform trace maintains crisp visibility at smaller scale
- Source labels truncate elegantly with ellipsis when needed

The sidebar remains fully expanded showing the complete oscillator list with color-coded entries matching each visible waveform. Scroll indicators appear on both the oscillator list and the main pane area. Performance metrics in status bar confirm smooth operation despite multiple active visualizations.

Complex multi-track audio interface, professional mastering workflow, grid layout system, 4K resolution, information-dense yet organized design.

---

## 5. Settings Dropdown Panel

A professional audio plugin interface shows an open settings dropdown panel floating elegantly above the main oscilloscope display. The dropdown panel features a dark slate (#1E293B) background with subtle border radius (8px) and soft shadow (0px 8px 24px rgba(0,0,0,0.4)) creating depth against the underlying interface.

**Panel contents organized in clear sections:**
- "Theme" section header in muted gray (#94A3B8) uppercase text
- Theme selection list showing "Dark Professional" (selected, highlighted in blue #3B82F6), "Light Modern", "High Contrast", "Classic Green" with hover states
- Divider line in charcoal (#334155)
- "Layout" section with three clickable column icons (1, 2, 3 columns) with the current selection highlighted
- "Status Bar" toggle switch in on position, showing green (#22C55E) active state
- "Theme Editor..." button at bottom in subtle outline style

The underlying oscilloscope remains visible but slightly dimmed, maintaining context while focusing attention on the dropdown. The settings gear icon in the header bar appears in active/pressed state (#3B82F6).

Floating dropdown UI pattern, Material Design 3 influence, 4K resolution, subtle animation-ready states, modern plugin settings interface.

---

## 6. Theme Editor Modal

A sophisticated theme customization modal overlays the main plugin interface with a semi-transparent dark backdrop (#000000 at 60% opacity). The modal panel in dark charcoal (#242438) features rounded corners (12px) and substantial shadow creating clear separation from the background.

**Modal layout in two sections:**

Left preview section displays a live miniature of the oscilloscope interface updating in real-time as colors change. A sample waveform in the preview shows the currently selected waveform color. Grid lines, background, and text all reflect current theme settings.

Right control section presents:
- "Theme Editor" title in white (#F8FAFC) with close X button
- "Theme Name" text input field containing "Custom Dark"
- Color picker rows for: Background (#1A1A2E), Grid (#2D3748), Text (#E2E8F0), Accent (#3B82F6)
- "Waveform Colors" section with 8 color swatches in a horizontal row
- Active color showing expanded picker with hue slider and saturation square
- "Save Theme" primary button (#3B82F6) and "Cancel" secondary button (outline style)

The color picker shows a selected electric cyan (#00D9FF) with the hex value displayed in an editable text field below the picker.

Professional theme customization interface, color picker UI patterns, 4K resolution, real-time preview capability, modern design tool aesthetic.

---

## 7. Oscillator Configuration Popup

A detailed oscillator configuration popup appears adjacent to a selected oscillator in the sidebar list, connected by a subtle triangle pointer. The popup panel in dark slate (#1E293B) with rounded corners (8px) presents comprehensive controls for waveform customization.

**Configuration sections:**

Header row displays oscillator name "Lead Vocal" as an editable text field with a colored dot (#00D9FF) indicating the current waveform color.

Source selection dropdown shows "Track 1 - Main Vocal" as the selected source with a downward chevron indicating expandability.

Processing Mode selector presents six options in a segmented button group: "Full Stereo" (selected, highlighted #3B82F6), "Mono", "Mid", "Side", "Left", "Right" in equal-width segments.

Color picker shows a horizontal palette of 16 preset colors plus a custom color button with color wheel icon.

Visual parameters section contains:
- "Line Width" slider (0.5-5.0) with current value "2.0px" displayed
- "Opacity" slider (0-100%) with current value "100%"
- "Vertical Scale" slider with "1.0x" displayed
- "Vertical Offset" slider centered at "0.0"

Pane assignment dropdown shows "Pane 1" with option to "Create New Pane..."

Footer contains "Delete Oscillator" button in red (#EF4444) text and "Close" button.

Inline configuration popup pattern, comprehensive audio controls, 4K resolution, organized parameter layout, professional DAW plugin interface.

---

## 8. Processing Modes Comparison View

A technical demonstration interface displays six oscilloscope panes in a 2x3 grid, each showing the same audio source processed through different signal modes. Deep charcoal (#1A1A2E) background unifies the comparison layout.

**Top row (stereo-based modes):**
- "Full Stereo" pane shows dual waveform traces - left channel in electric cyan (#00D9FF), right channel in coral (#FB7185) - clearly separated vertically
- "Mid" pane displays single centered trace in amber (#F59E0B) showing the mono sum (L+R)/2
- "Side" pane shows single trace in purple (#A855F7) representing stereo difference (L-R)/2

**Bottom row (channel isolation):**
- "Mono" pane displays combined mono signal in lime green (#39FF14)
- "Left" pane shows only left channel extraction in electric cyan (#00D9FF)
- "Right" pane shows only right channel extraction in coral (#FB7185)

Each pane header clearly labels the processing mode with matching icon. The same audio moment is captured across all views for direct comparison. Grid lines remain consistent at 10% opacity slate (#2D3748) across all panes. Amplitude scales match for accurate visual comparison.

Educational audio visualization, signal processing demonstration, 4K resolution, technical documentation quality, professional audio engineering interface.

---

## 9. Timing Configuration Panel

The sidebar timing configuration section expands to reveal comprehensive synchronization controls against dark charcoal (#242438) background. Section header "Timing" in white (#F8FAFC) with a small clock icon sits above the controls.

**Mode selector:**
Segmented control showing "TIME" and "MELODIC" modes with MELODIC currently selected (highlighted in blue #3B82F6).

**MELODIC mode controls visible:**
- "Note Interval" dropdown displaying "1/4 Note" with musical note icon
- Full list visible showing: 1/16th, 1/8th, 1/4, 1/2, 1 Bar, 2 Bars, 4 Bars
- "Host BPM" read-only field showing "120.0" with lock icon indicating DAW control
- Calculated display: "= 500ms" in muted text showing actual interval duration

**Sync options:**
- "Sync to Host" toggle switch in active green (#22C55E) position
- "Reset on Play" checkbox checked
- Visual indicator showing "SYNCED" status badge in green

The oscilloscope area behind shows waveforms with visible beat grid markers - thin vertical lines in amber (#F59E0B with 30% opacity) aligned to quarter note boundaries, demonstrating the active musical synchronization.

Music-aware timing controls, DAW integration interface, 4K resolution, professional MIDI/sync aesthetic, audio production workflow.

---

## 10. Pane Drag-and-Drop in Progress

A dynamic interface state captures the moment of pane reorganization through drag-and-drop interaction. The three-column layout shows active reordering with clear visual feedback against dark charcoal (#1A1A2E) background.

**Drag state visualization:**
- The dragged pane "Drums" appears semi-transparent (50% opacity) floating above the layout with a subtle blue glow (#3B82F6 at 30% opacity) around its edges
- A cursor-attached ghost preview shows the pane following mouse position
- Original position displays a dashed outline placeholder in slate (#334155)

**Drop target indication:**
- Target column highlights with a subtle blue tint (#3B82F6 at 10% opacity)
- Drop position indicator shows as a horizontal line in bright blue (#3B82F6) with animated pulse effect
- Adjacent panes show preview of how they will shift with subtle animation arrows

**Other panes remain stable:**
- Non-participating panes display at full opacity
- Their waveforms continue rendering in real-time
- Visual hierarchy clearly separates dragged element from static content

The cursor displays as a grabbing hand icon. Status message at bottom reads "Drop to reorder" in muted text.

Drag-and-drop UI interaction, real-time state feedback, 4K resolution, motion-ready design, fluid interaction design.

---

## 11. Empty State / Getting Started

A welcoming empty state interface guides new users when no oscillators have been created yet. The main oscilloscope area displays a centered instructional panel against the dark charcoal (#1A1A2E) background with subtle radial gradient creating depth.

**Central welcome content:**
- Large oscilloscope icon (stylized waveform graphic) in muted slate (#475569) at 64px size
- "Welcome to Oscil" heading in white (#F8FAFC) at 24px
- Subtext "Multi-Track Oscilloscope for Professional Audio Analysis" in muted gray (#94A3B8)
- Three-step quick start list:
  1. "Add this plugin to DAW tracks you want to visualize"
  2. "Open the UI on any instance to see all sources"
  3. "Click 'Add Oscillator' to start analyzing"
- Primary action button "Add Your First Oscillator" in bright blue (#3B82F6) with plus icon

**Sidebar state:**
- Empty oscillator list shows "No oscillators yet" placeholder text
- "Add Oscillator" button prominently available and pulsing subtly
- Available Sources section lists detected DAW tracks ready for selection
- Settings and theme options remain accessible

The interface feels inviting rather than broken, guiding users toward productive first actions.

Onboarding empty state, user guidance pattern, 4K resolution, welcoming first-run experience, professional software introduction.

---

## 12. Source Selection Dropdown

An expanded source selection dropdown presents all available audio sources detected across the DAW session. The dropdown panel in dark slate (#1E293B) floats below the "Source" field in the oscillator configuration popup.

**Dropdown structure:**
- Search field at top with magnifying glass icon and "Filter sources..." placeholder text
- Section header "Available Sources" in uppercase muted gray (#64748B)

**Source list items displaying:**
- "Track 1 - Lead Vocal" with microphone icon, stereo indicator badge
- "Track 2 - Drums" with drum icon, stereo indicator
- "Track 3 - Bass DI" with bass guitar icon, mono indicator badge
- "Track 4 - Synth Pad" with keyboard icon, stereo indicator
- "Track 5 - Acoustic Guitar" with guitar icon, stereo indicator
- "Master Bus" with master fader icon, stereo indicator
- Currently selected item "Track 1 - Lead Vocal" shows blue (#3B82F6) background highlight

**Visual indicators:**
- Each source shows a small activity indicator - green dot (#22C55E) for active audio, gray (#6B7280) for silent
- Stereo/mono badges in small pill shapes
- Hover state shows subtle highlight (#334155)
- Scroll bar appears when list exceeds visible area

At bottom, "No Source (Disconnect)" option in muted style allows removing the source assignment.

Source selection interface, audio routing UI, 4K resolution, DAW integration design, professional audio software pattern.

---

## 13. Performance Warning State

A critical interface state displays when system resources approach concerning levels. The main oscilloscope continues rendering but with visual indicators of performance strain.

**Warning overlay:**
A dismissible notification banner spans the top of the oscilloscope area in warning amber (#F59E0B) with alert triangle icon. Message reads "High CPU Usage Detected (12.4%) - Consider reducing quality or disabling oscillators" in dark text on amber background.

**Status bar changes:**
- CPU metric "CPU: 12.4%" now displays in amber (#F59E0B) text instead of standard gray
- Memory metric shows normally "Memory: 487MB"
- FPS indicator "52 FPS" shows slight amber tint indicating below target
- Small "Reduce Quality" quick action button appears in status bar

**Visual quality indicators:**
- Waveforms render with visible decimation (slightly stepped rather than smooth)
- Frame rate reduction visible as slight motion judder
- One oscillator shows "PAUSED" badge indicating automatic quality reduction kicked in

**Action dropdown accessible:**
Quality settings shortcut showing: "High Quality", "Balanced" (current), "Performance Mode", "Maximum Performance" options with descriptions of trade-offs.

Performance monitoring interface, resource warning pattern, 4K resolution, graceful degradation feedback, professional system health indication.

---

## 14. Status Bar Detail View

A focused view of the plugin status bar presents comprehensive system monitoring information in an expanded hover state. The status bar spans the full window width at bottom against slightly darker charcoal (#151521) background.

**Status bar sections from left to right:**

**Performance metrics cluster:**
- CPU icon followed by "4.2%" in white (#F8FAFC), green (#22C55E) status dot
- Memory icon followed by "287 MB" in white
- Vertical divider line (#334155)
- "60 FPS" with small sparkline showing frame time consistency
- "OpenGL" badge in green outline indicating hardware acceleration active

**Sync status cluster:**
- "HOST SYNC" badge in blue (#3B82F6) when active
- "120 BPM" from DAW
- Transport indicator showing play/stop state with animation

**Instance info (far right):**
- "Instance 3 of 5" showing multi-instance context
- "5 Sources Active" count
- Plugin version "v1.2.0" in muted text

**Hover expansion state:**
When hovering, status bar expands upward revealing additional details:
- CPU graph showing last 30 seconds
- Memory allocation breakdown
- Frame time histogram
- Network latency to other instances

Comprehensive status monitoring, system telemetry display, 4K resolution, professional DAW status bar pattern, information-dense footer design.

---

## 15. Multi-Pane Waveform Detail

A close-up technical view focuses on oscilloscope waveform rendering quality and detail. Three panes display different audio characteristics against dark charcoal (#1A1A2E) with emphasis on waveform precision.

**Top pane - Transient material (Drums):**
Sharp, dynamic waveform in hot magenta (#FF006E) showing clear attack transients with steep vertical rises. Peak levels reach near clipping with distinct shape definition. The waveform captures a snare hit with visible fast attack and controlled decay.

**Middle pane - Sustained material (Synth Pad):**
Smooth, flowing waveform in purple (#A855F7) displaying gradual amplitude changes and organic modulation. The oscillating pattern shows clear harmonic content with subtle beating effects visible in the amplitude envelope.

**Bottom pane - Low frequency material (Bass):**
Bold, slow-moving waveform in lime green (#39FF14) with clear fundamental frequency visible as large, smooth undulations. Higher harmonic content creates subtle texture on top of the fundamental wave shape.

**Rendering details visible:**
- Anti-aliased waveform edges with no stepping artifacts
- Subtle glow effect around bright peaks
- Grid lines perfectly aligned at standard intervals
- Time axis markers showing millisecond values
- Amplitude scale showing dB values on left edge
- Precise intersection of waveform with grid for measurement

Technical waveform visualization, audio analysis detail, 4K resolution, oscilloscope precision rendering, professional metering aesthetic.

---

## 16. Oscillator List with Multiple Items

A detailed view of the sidebar oscillator list shows comprehensive management of multiple audio visualization channels. The list container in dark charcoal (#242438) displays clearly organized oscillator entries.

**List toolbar:**
- Sort dropdown showing "By Source" with arrow indicating ascending order
- Filter toggle buttons: "All", "Visible", "Hidden"
- Total count badge "8 Oscillators"

**Oscillator list items (scrollable):**

| Item | Color Dot | Name | Source | Actions |
|------|-----------|------|--------|---------|
| 1 | #00D9FF | "Lead Vocal" | Track 1 | Config, Delete icons |
| 2 | #FF006E | "Drums - Kick" | Track 2 | Config, Delete icons |
| 3 | #39FF14 | "Bass DI" | Track 3 | Config, Delete icons |
| 4 | #F59E0B | "Acoustic Gtr" | Track 4 | Config, Delete icons |
| 5 | #A855F7 | "Synth Pad L" | Track 5 | Config, Delete icons |
| 6 | #FB7185 | "Synth Pad R" | Track 5 | Config, Delete icons |
| 7 | #22D3EE | "Room Mic" | Track 6 | Config, Delete icons (dimmed - hidden) |
| 8 | #FBBF24 | "FX Return" | Aux 1 | Config, Delete icons |

**Selected item (Lead Vocal):**
Shows highlighted background (#334155) with expanded quick controls visible - processing mode selector and visibility toggle.

**Visual states:**
- Hover state shows subtle highlight
- Hidden oscillator (#7) appears dimmed with eye-slash icon
- Drag handle visible on left edge of each item

Oscillator management list, audio channel organization, 4K resolution, list UI patterns, professional mixer-style interface.

---

## Notes for Image Generation

### Consistent Design Tokens

Use these values across all prompts for visual consistency:

**Colors:**
- Background Primary: #1A1A2E
- Background Secondary: #242438
- Background Tertiary: #1E293B
- Accent Blue: #3B82F6
- Success Green: #22C55E
- Warning Amber: #F59E0B
- Error Red: #EF4444
- Text Primary: #F8FAFC
- Text Secondary: #94A3B8
- Border: #334155

**Waveform Palette:**
- Cyan: #00D9FF
- Magenta: #FF006E
- Green: #39FF14
- Amber: #F59E0B
- Purple: #A855F7
- Coral: #FB7185
- Sky: #38BDF8
- Yellow: #FBBF24

**Typography:**
- Clean sans-serif throughout
- Monospace for metrics and values
- Bold for headers, regular for content

**Spacing:**
- 8px grid system
- 16px standard padding
- 4px small gaps
- 24px section spacing

### Recommended Generation Settings

- Resolution: 4K (3840x2160) or 2K (1920x1080)
- Aspect ratio: 16:9 for full interface, 4:3 for focused sections
- Style consistency: Run multiple generations and select most consistent results
- Quality words to include: "professional", "4K resolution", "JUCE framework", "DAW plugin"
