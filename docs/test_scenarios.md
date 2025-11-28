# Oscil Plugin - End-to-End Test Scenarios

## Overview

This document defines comprehensive user journey test scenarios for the Oscil multi-track oscilloscope plugin. Each scenario represents a complete user workflow, not just isolated interactions.

**Testing Philosophy**: Every user-visible element must be correct and themed. Every user action must work completely. Tests follow complete journeys from trigger to final verification.

## Test Environment Prerequisites

- DAW with VST3/AU support (Ableton, Logic, Reaper, Bitwig, Cubase)
- At least 2 audio tracks with different audio content
- Plugin instance loaded on at least one track
- Audio playback capability

---

## Category 1: Plugin Initialization & Basic Display

### TC-INIT-001: Fresh Plugin Load

**Description**: Verify plugin loads correctly with default state on first use.

**Preconditions**:
- No previous Oscil state saved in project
- Clean DAW project

**User Actions**:
1. Create new audio track
2. Insert Oscil plugin on track
3. Open plugin UI

**Expected Results**:
- Plugin window opens at default size (1200x800)
- Sidebar visible on right side at default width (280px)
- Default "Dark Professional" theme applied
- Single pane visible in main area
- Status bar visible at bottom showing: FPS, CPU %, Memory MB, Rendering mode
- Toolbar visible at top with: Column selector (1 selected), Add Oscillator button, Theme dropdown, Sidebar toggle
- Source appears in sidebar Sources section with track name

**Verification Points**:
- [ ] Window dimensions correct
- [ ] All theme colors consistent with "Dark Professional"
- [ ] No visual artifacts or misaligned components
- [ ] Source discovered and listed
- [ ] Status bar metrics updating (FPS should be non-zero when playing)

---

### TC-INIT-002: Plugin Load with Saved State

**Description**: Verify plugin restores saved state correctly when reopening project.

**Preconditions**:
- Previously saved project with Oscil configuration
- Saved state includes: 2 oscillators, custom theme, 2-column layout, sidebar collapsed

**User Actions**:
1. Open saved DAW project
2. Open Oscil plugin UI

**Expected Results**:
- All 2 oscillators visible in their respective panes
- Custom theme colors applied throughout UI
- 2-column layout active (column selector shows 2 selected)
- Sidebar is collapsed (only collapse button visible)
- All oscillator properties restored (colors, modes, visibility)
- Timing settings restored to saved values
- Display options restored (grid, autoscale, hold states)

**Verification Points**:
- [ ] Oscillator count matches saved (2)
- [ ] Theme name in dropdown matches saved custom theme
- [ ] Column layout matches saved (2)
- [ ] Sidebar collapsed state matches saved (collapsed)
- [ ] Each oscillator color matches saved
- [ ] Timing mode and interval match saved

---

## Category 2: Source Discovery & Management

### TC-SRC-001: Automatic Source Discovery

**Description**: Verify sources are automatically discovered when plugin instances are added to DAW tracks.

**Preconditions**:
- Open Oscil instance (aggregator) on track 1
- No other Oscil instances

**User Actions**:
1. Create new audio track (track 2)
2. Add audio content to track 2
3. Insert Oscil plugin on track 2
4. Return to track 1's Oscil UI

**Expected Results**:
- Track 1's Oscil now shows 2 sources in sidebar Sources section
- Each source shows: Track name (from DAW), Activity indicator, "Add to Pane" dropdown
- Sources are uniquely identified (no duplicates)
- Source list updates automatically without manual refresh

**Verification Points**:
- [ ] Both sources visible in Sources list
- [ ] Source names match DAW track names
- [ ] Each source has Add to Pane dropdown
- [ ] No duplicate sources appear

---

### TC-SRC-002: Source Removal

**Description**: Verify source removal is handled gracefully when plugin instance is removed.

**Preconditions**:
- Oscil on track 1 showing oscillator from track 2's source
- Track 2 has Oscil instance

**User Actions**:
1. Remove Oscil plugin from track 2
2. Observe track 1's Oscil UI

**Expected Results**:
- Source from track 2 disappears from Sources list
- Oscillator using track 2's source shows "No Source" or placeholder state
- Oscillator is NOT automatically deleted (preserves user configuration)
- Waveform shows flat line (no audio data)
- Source can be reassigned later when track 2's Oscil is re-added

**Verification Points**:
- [ ] Source removed from list
- [ ] Oscillator still exists
- [ ] Oscillator configuration preserved (color, mode, etc.)
- [ ] No crash or error on source removal

---

## Category 3: Oscillator Creation & Configuration

### TC-OSC-001: Create Oscillator via Add Button

**Description**: Complete journey of creating an oscillator from the toolbar Add button.

**Preconditions**:
- At least one source available
- Plugin UI open

**User Actions**:
1. Click "Add Oscillator" button in toolbar
2. Observe new oscillator appears
3. Select a source for the oscillator (via config popup or source selector)

**Expected Results**:
- New oscillator row appears in sidebar Oscillators section
- New waveform trace appears in pane
- Oscillator has default color from theme's waveform color palette
- Oscillator is visible by default
- Processing mode defaults to "FullStereo"
- Waveform displays audio from selected source

**Verification Points**:
- [ ] Oscillator visible in sidebar list
- [ ] Waveform visible in pane
- [ ] Color matches theme's next available waveform color
- [ ] Waveform shows actual audio content when playing

---

### TC-OSC-002: Create Oscillator from Source

**Description**: Complete journey of creating an oscillator directly from a source in the sidebar.

**Preconditions**:
- At least one source available with audio
- Plugin UI open

**User Actions**:
1. Locate source in sidebar Sources section
2. Click "Add to Pane" dropdown on source
3. Select "Pane 1" (or create new pane)

**Expected Results**:
- New oscillator created and assigned to selected pane
- Oscillator automatically linked to that source
- Waveform immediately shows source's audio content
- Oscillator appears in Oscillators list in sidebar

**Verification Points**:
- [ ] Oscillator created in correct pane
- [ ] Source linked correctly
- [ ] Waveform shows audio immediately (no manual source selection needed)

---

### TC-OSC-003: Configure Oscillator via Config Popup

**Description**: Complete journey of configuring all oscillator properties via the config popup.

**Preconditions**:
- Existing oscillator with source assigned
- Audio playing

**User Actions**:
1. Click the gear/config icon on oscillator row in sidebar
2. Config popup opens
3. Change name from default to "Vocals"
4. Toggle visibility off, then on
5. Change source to different available source
6. Select "Mid" processing mode
7. Select new color from color swatches (e.g., blue)
8. Adjust Line Width slider to 3.0
9. Adjust Opacity slider to 75%
10. Adjust Vertical Scale to 1.5x
11. Adjust Vertical Offset to +0.2
12. Close popup

**Expected Results after each step**:
- Step 3: Name updates in sidebar oscillator list
- Step 4 (off): Waveform disappears from pane
- Step 4 (on): Waveform reappears
- Step 5: Waveform shows new source's audio
- Step 6: Waveform changes from stereo (L/R stacked) to single mid trace
- Step 7: Waveform color changes to blue immediately
- Step 8: Waveform line becomes thicker
- Step 9: Waveform becomes 75% opaque (slightly transparent)
- Step 10: Waveform amplitude increased (peaks reach 1.5x higher)
- Step 11: Waveform shifted up by 20% of pane height
- Step 12: Popup closes, all settings persist

**Verification Points**:
- [ ] Each property change applies immediately (real-time preview)
- [ ] Sidebar list reflects name change
- [ ] Processing mode label updates in sidebar
- [ ] All changes persist after closing popup
- [ ] Changes persist after closing and reopening plugin

---

### TC-OSC-004: Processing Mode Changes

**Description**: Verify all 6 processing modes display correctly.

**Preconditions**:
- Oscillator with stereo source
- Audio playing

**User Actions** (for each mode):
1. Open oscillator config popup
2. Select processing mode from segmented buttons
3. Observe waveform in pane
4. Close popup

**Expected Results per mode**:
- **FullStereo**: Two waveforms stacked (L on top, R on bottom), labeled "L" and "R"
- **Mono**: Single waveform showing (L+R)/2 sum
- **Mid**: Single waveform showing (L+R)/2 (identical to Mono for typical content)
- **Side**: Single waveform showing (L-R)/2 (shows stereo difference, may be quieter)
- **Left**: Single waveform showing left channel only
- **Right**: Single waveform showing right channel only

**Verification Points**:
- [ ] FullStereo shows 2 separate traces
- [ ] FullStereo has L/R labels visible
- [ ] Mono shows single centered trace
- [ ] Side shows difference (silence for mono-compatible content)
- [ ] Left shows left channel only
- [ ] Right shows right channel only

---

### TC-OSC-005: Delete Oscillator

**Description**: Complete journey of deleting an oscillator.

**Preconditions**:
- At least 2 oscillators exist
- One oscillator selected for deletion

**User Actions**:
1. Open config popup for oscillator to delete
2. Click "Delete" button at bottom of popup
3. Confirm deletion (if confirmation appears)

**Expected Results**:
- Oscillator removed from sidebar Oscillators list
- Waveform removed from pane
- Other oscillators unaffected
- Deletion persists after closing/reopening plugin
- If deleted oscillator was only one in pane, pane may show empty or be removed

**Verification Points**:
- [ ] Oscillator removed from list
- [ ] Waveform removed from display
- [ ] No residual UI artifacts
- [ ] State persists correctly

---

### TC-OSC-006: Oscillator Visibility Toggle

**Description**: Verify visibility toggle hides/shows waveform correctly.

**Preconditions**:
- Oscillator with visible waveform

**User Actions**:
1. Click visibility toggle (eye icon) in sidebar oscillator row
2. Observe waveform
3. Click visibility toggle again

**Expected Results**:
- Step 1: Waveform disappears from pane, toggle shows "hidden" state
- Step 3: Waveform reappears, toggle shows "visible" state
- Oscillator row styling may change (dimmed when hidden)
- Hidden oscillators still process audio (for when shown again)

**Verification Points**:
- [ ] Waveform visibility matches toggle state
- [ ] Toggle visual state is clear (visible vs hidden)
- [ ] Row styling indicates hidden state

---

### TC-OSC-007: Oscillator Reordering

**Description**: Verify oscillators can be reordered via drag-and-drop.

**Preconditions**:
- At least 3 oscillators in list
- All in same pane

**User Actions**:
1. Drag oscillator row from position 3 to position 1
2. Release

**Expected Results**:
- Oscillator moves to new position in sidebar list
- Order persists after closing popup
- Order persists after save/load
- Waveform z-order in pane may change (front-to-back rendering)

**Verification Points**:
- [ ] Order change reflected in sidebar list
- [ ] Drag preview visible during drag
- [ ] Order persists after plugin reopen

---

### TC-OSC-008: Oscillator Selection and Expanded Controls

**Description**: Verify clicking an oscillator item in sidebar reveals mode change and visibility controls.

**Preconditions**:
- At least 2 oscillators in sidebar list
- No oscillator currently selected

**User Actions**:
1. Observe oscillator items in sidebar (compact state, ~56px height each)
2. Click on first oscillator item
3. Observe expanded state with controls
4. Change processing mode by clicking a mode button (e.g., "Mono")
5. Toggle visibility by clicking eye icon
6. Click on second oscillator item
7. Click elsewhere (deselect)

**Expected Results**:
- **Step 1**: Items show compact view: color indicator, name, source name, settings gear icon
- **Step 2**: Clicked item expands to ~100px height, reveals additional controls
- **Step 3**: Expanded view shows:
  - Processing mode buttons (Stereo, Mono, Mid, Side, L, R)
  - Current mode button highlighted
  - Visibility toggle (eye icon)
  - Delete button
- **Step 4**: Mode changes immediately:
  - Button highlight moves to new mode
  - Waveform in pane updates to show new processing
  - If switching from Stereo to Mono, dual traces become single trace
- **Step 5**: Visibility toggles:
  - Eye icon changes state (open/closed)
  - Waveform in pane hides/shows
  - Item row may dim when hidden
- **Step 6**: First item collapses, second item expands
- **Step 7**: All items return to compact state

**Verification Points**:
- [ ] Compact height is ~56px, expanded is ~100px
- [ ] Mode buttons visible only when selected
- [ ] Mode change reflects in waveform immediately
- [ ] Visibility toggle reflects in pane immediately
- [ ] Only one item can be selected/expanded at a time
- [ ] Click outside deselects all items

---

### TC-OSC-009: Oscillator Drag Handle in Sidebar

**Description**: Verify oscillator reordering uses correct drag handle zone.

**Preconditions**:
- At least 3 oscillators in sidebar list
- Oscillators visible (not filtered)

**User Actions**:
1. Hover over left edge of oscillator item (drag handle zone, ~24px)
2. Cursor changes to grab cursor
3. Click and begin dragging
4. Drag preview appears
5. Drag up/down to different position
6. Drop indicator shows insertion point between items
7. Release to drop
8. Try dragging from non-drag zone (center of item)

**Expected Results**:
- **Step 1-2**: Drag handle zone indicated by cursor change
- **Step 3-4**: Item follows cursor with drag preview (semi-transparent)
- **Step 5-6**: Other items shift to show drop target position
- **Step 7**: Item moves to new position, list reorders
- **Step 8**: Clicking center of item selects it (does not start drag)

**Verification Points**:
- [ ] Grab cursor only in drag handle zone
- [ ] Drag preview follows cursor smoothly
- [ ] Drop indicator visible between valid targets
- [ ] Reorder persists after save/load
- [ ] Non-drag zones do not initiate drag

---

## Category 4: Theme Management

### TC-THM-001: Switch Theme

**Description**: Complete journey of switching between themes.

**Preconditions**:
- Multiple themes available (at least "Dark Professional" and "Light Classic")
- Plugin UI open

**User Actions**:
1. Click Theme dropdown in toolbar
2. Select "Light Classic" theme

**Expected Results**:
- All UI colors update immediately (within one frame)
- Background colors change (dark to light)
- Text colors change (light to dark for contrast)
- Grid colors change appropriately
- Control colors update
- Waveform colors may update if theme-linked
- Status bar colors update
- Sidebar colors update
- No visual artifacts during transition

**Verification Points**:
- [ ] Background color matches Light Classic theme
- [ ] Text is readable (good contrast)
- [ ] All UI sections updated (sidebar, toolbar, panes, status bar)
- [ ] Theme selection persists after close/reopen

---

### TC-THM-002: Create Custom Theme

**Description**: Complete journey of creating a new custom theme.

**Preconditions**:
- Plugin UI open
- At least one system theme exists

**User Actions**:
1. Open Settings popup (click gear icon or dropdown)
2. Click "Edit Theme" button
3. Theme Editor window opens
4. Click "Create" button
5. Enter name "My Custom Theme" in name field
6. Modify background color (click Background Primary swatch)
7. Pick a purple color in color picker
8. Close color picker
9. Click "Apply" to apply theme
10. Close Theme Editor

**Expected Results**:
- New theme appears in theme list
- Theme name "My Custom Theme" visible
- Modified background color applied to UI
- Theme is selected and active
- Theme persists in theme list after plugin restart
- Theme available across all Oscil instances

**Verification Points**:
- [ ] Theme appears in list
- [ ] Custom name displayed correctly
- [ ] Background color changed to purple
- [ ] Theme saved and available on restart

---

### TC-THM-003: Clone Theme

**Description**: Complete journey of cloning an existing theme.

**Preconditions**:
- System theme "Dark Professional" exists

**User Actions**:
1. Open Theme Editor
2. Select "Dark Professional" in theme list
3. Click "Clone" button
4. New theme appears as "Dark Professional Copy"
5. Rename to "My Dark Theme"
6. Modify a color (e.g., controlActive to orange)
7. Click "Apply"

**Expected Results**:
- Cloned theme created with all colors from source
- Source theme unchanged
- Modified color applied
- Both themes available in list
- Cloned theme is custom (editable/deletable)

**Verification Points**:
- [ ] Clone created with unique name
- [ ] Source theme unchanged
- [ ] Clone is editable (not system theme)

---

### TC-THM-004: Delete Custom Theme

**Description**: Complete journey of deleting a custom theme.

**Preconditions**:
- Custom theme "My Test Theme" exists
- A different theme is currently active

**User Actions**:
1. Open Theme Editor
2. Select "My Test Theme"
3. Click "Delete" button
4. Confirm deletion

**Expected Results**:
- Theme removed from list
- Theme no longer available in dropdown
- If deleted theme was active, switches to default theme
- System themes still have Delete button disabled

**Verification Points**:
- [ ] Theme removed from list
- [ ] Cannot delete system themes
- [ ] Fallback to default if active theme deleted

---

### TC-THM-005: System Theme Protection

**Description**: Verify system themes cannot be edited or deleted.

**Preconditions**:
- System theme "Dark Professional" exists

**User Actions**:
1. Open Theme Editor
2. Select "Dark Professional"
3. Attempt to modify a color
4. Observe Delete button state

**Expected Results**:
- Color swatches are disabled or read-only for system themes
- Delete button is disabled
- "System Theme" indicator visible
- Apply button disabled (no changes possible)

**Verification Points**:
- [ ] Colors not editable for system themes
- [ ] Delete button disabled
- [ ] System theme indicator visible

---

### TC-THM-006: Theme Export/Import

**Description**: Complete journey of exporting and importing a theme.

**Preconditions**:
- Custom theme exists

**User Actions**:
1. Open Theme Editor
2. Select custom theme
3. Click "Export" button
4. Save to file (JSON format)
5. Delete the theme
6. Click "Import" button
7. Load the exported file

**Expected Results**:
- Export creates valid JSON file
- Import restores theme with all colors
- Imported theme appears in list
- All colors match original

**Verification Points**:
- [ ] Export creates readable JSON file
- [ ] Import restores theme correctly
- [ ] All color values match

---

## Category 5: Layout Management

### TC-LAY-001: Column Layout Switching

**Description**: Complete journey of switching between 1, 2, and 3 column layouts.

**Preconditions**:
- At least 3 panes with oscillators
- Currently in 1-column layout

**User Actions**:
1. Click Column selector, select 2 columns
2. Observe pane arrangement
3. Click Column selector, select 3 columns
4. Click Column selector, select 1 column

**Expected Results**:
- **2 columns**: Panes arranged in 2 columns, roughly equal width
- **3 columns**: Panes arranged in 3 columns
- **1 column**: Panes stacked vertically (default)
- Transitions are smooth (no jarring redraws)
- Layout selection persists after close/reopen

**Verification Points**:
- [ ] Correct number of columns displayed
- [ ] Panes distribute evenly across columns
- [ ] Layout persists in saved state

---

### TC-LAY-002: Sidebar Resize

**Description**: Complete journey of resizing the sidebar.

**Preconditions**:
- Sidebar expanded at default width

**User Actions**:
1. Hover over sidebar left edge (resize handle)
2. Cursor changes to resize cursor
3. Drag left to make sidebar wider
4. Drag right to make sidebar narrower
5. Release

**Expected Results**:
- Sidebar width changes during drag
- Main pane area adjusts accordingly
- Minimum width enforced (200px)
- Maximum width enforced (800px)
- New width persists after close/reopen

**Verification Points**:
- [ ] Resize cursor appears on hover
- [ ] Width changes smoothly during drag
- [ ] Minimum/maximum limits respected
- [ ] Width persists in saved state

---

### TC-LAY-003: Sidebar Collapse/Expand

**Description**: Complete journey of collapsing and expanding sidebar.

**Preconditions**:
- Sidebar expanded

**User Actions**:
1. Click sidebar toggle button in toolbar (or collapse button on sidebar)
2. Sidebar collapses
3. Click again to expand

**Expected Results**:
- **Collapsed**: Sidebar hidden (or minimal width), main area expands
- **Expanded**: Sidebar returns to previous width
- Collapse button arrow direction changes
- Animation smooth (if animations enabled)
- Collapse state persists after close/reopen

**Verification Points**:
- [ ] Sidebar fully collapses
- [ ] Main area uses full width when collapsed
- [ ] Expand restores previous width
- [ ] State persists

---

### TC-LAY-004: Pane Drag Within Same Column

**Description**: Verify panes can be reordered within a single column via drag-and-drop.

**Preconditions**:
- Single-column layout selected
- At least 3 panes (stacked vertically)

**User Actions**:
1. Hover over pane header area of bottom pane
2. Click and hold on pane header (drag zone)
3. Drag upward toward top of viewport
4. Observe drop indicator between other panes
5. Position between pane 1 and pane 2
6. Release to drop

**Expected Results**:
- **Step 1-2**: Cursor indicates draggable area on pane header
- **Step 3**: Pane visually lifts (shadow/opacity change), follows cursor
- **Step 4**: Drop indicator line appears between valid drop targets
- **Step 5**: Drop indicator highlights position between panes 1 and 2
- **Step 6**: Pane moves to new position, former pane 2 shifts down
- Pane heights redistribute evenly in column
- Order persists after save/load

**Verification Points**:
- [ ] Drag only initiates from pane header (not waveform area)
- [ ] Drag preview follows cursor smoothly
- [ ] Drop indicator visible at valid positions
- [ ] Pane cannot be dropped onto itself
- [ ] New order reflected immediately
- [ ] Order persists after plugin reopen

---

### TC-LAY-005: Pane Drag Across Columns (Multi-Column Layout)

**Description**: Verify panes can be moved between columns in multi-column layout.

**Preconditions**:
- 2-column or 3-column layout selected
- At least 4 panes distributed across columns
- Example: Column 1 has panes A, B; Column 2 has panes C, D

**User Actions**:
1. Set layout to 2 columns (verify panes distributed)
2. Drag pane from column 1 toward column 2
3. Observe drop indicator appears in column 2
4. Drop between panes C and D in column 2
5. Verify column 1 now has fewer panes, column 2 has more
6. Drag pane back to column 1
7. Change to 3-column layout and repeat drag between columns

**Expected Results**:
- **Step 2**: Pane follows cursor across column boundary
- **Step 3**: Drop indicator appears in target column
- **Step 4**: Pane moves from column 1 to column 2
- **Step 5**:
  - Column 1: Remaining panes expand to fill space
  - Column 2: Panes resize to accommodate new pane
- **Step 6**: Pane returns to original column
- **Step 7**: Drag works across all 3 columns

**Verification Points**:
- [ ] Pane can be dragged across column boundaries
- [ ] Drop indicator shows in correct column
- [ ] Source column panes resize after pane removed
- [ ] Target column panes resize to fit new pane
- [ ] Order within target column respects drop position
- [ ] Works for 2-column and 3-column layouts
- [ ] Order persists after save/load

---

### TC-LAY-006: Pane Drag Edge Cases

**Description**: Verify pane drag-and-drop handles edge cases correctly.

**Preconditions**:
- At least 2 panes

**User Actions**:
1. Start dragging a pane, then press Escape
2. Start dragging, move outside plugin window, release
3. Try to drag pane onto its current position
4. Drag the only pane in a column to another column (leaving source column empty)
5. Drag pane to very top/bottom of column (first/last position)

**Expected Results**:
- **Step 1**: Drag cancelled, pane returns to original position
- **Step 2**: Drag cancelled, pane returns to original position
- **Step 3**: No reorder occurs (pane stays in place)
- **Step 4**: Empty column handled gracefully (collapses or shows placeholder)
- **Step 5**: Pane correctly positioned at boundary positions

**Verification Points**:
- [ ] Escape cancels drag operation
- [ ] Mouse release outside window cancels drag
- [ ] Self-drop is no-op
- [ ] Empty columns handled without crash
- [ ] Boundary positions work correctly

---

## Category 6: Timing Configuration

### TC-TIM-001: TIME Mode Configuration

**Description**: Complete journey of configuring TIME mode timing.

**Preconditions**:
- Plugin open, audio playing

**User Actions**:
1. Expand Timing section in sidebar
2. Ensure "TIME" mode is selected
3. Adjust time interval slider to 100ms
4. Observe waveform display

**Expected Results**:
- Time interval value updates as slider moves
- Waveform display window changes (shows 100ms of audio)
- Waveform updates at new interval
- Time value shown in ms (e.g., "100 ms")

**Verification Points**:
- [ ] Slider updates interval value
- [ ] Waveform window duration changes
- [ ] Value displayed in UI

---

### TC-TIM-002: MELODIC Mode Configuration

**Description**: Complete journey of configuring MELODIC mode timing.

**Preconditions**:
- DAW playing at 120 BPM

**User Actions**:
1. Expand Timing section
2. Select "MELODIC" mode toggle
3. Select "1/4 Note" from note interval dropdown
4. Observe calculated interval display
5. Select "1/8 Note"
6. Observe calculated interval update

**Expected Results**:
- Mode switches to MELODIC
- Note interval dropdown becomes visible/active
- Time interval slider becomes hidden/inactive
- Calculated interval shows (at 120 BPM):
  - 1/4 note = 500ms
  - 1/8 note = 250ms
- BPM display shows "120 BPM" (from host)

**Verification Points**:
- [ ] Mode toggle works
- [ ] Correct controls visible for each mode
- [ ] Calculated interval matches formula: (60000 / BPM) * beats
- [ ] BPM displayed from host

---

### TC-TIM-003: Host Sync Enable/Disable

**Description**: Verify host sync functionality.

**Preconditions**:
- DAW with transport controls

**User Actions**:
1. Enable "Host Sync" toggle in Timing section
2. Start DAW playback
3. Observe sync status indicator
4. Stop DAW playback
5. Disable "Host Sync" toggle

**Expected Results**:
- Sync indicator shows "Synced" when playing
- Waveform resets align with host events
- Sync indicator shows "Not Synced" when stopped or disabled
- Waveform continues independently when sync disabled

**Verification Points**:
- [ ] Sync toggle state changes
- [ ] Status indicator reflects sync state
- [ ] Waveform behavior changes with sync on/off

---

### TC-TIM-004: Reset on Play

**Description**: Verify reset on play functionality.

**Preconditions**:
- Host Sync enabled

**User Actions**:
1. Enable "Reset on Play" toggle
2. Stop DAW playback
3. Start DAW playback
4. Observe waveform

**Expected Results**:
- Waveform resets/restarts when playback begins
- Clean start of waveform capture
- No leftover audio from before playback

**Verification Points**:
- [ ] Waveform resets on play start
- [ ] No stale data visible

---

## Category 7: Display Options

### TC-DIS-001: Show Grid Toggle

**Description**: Verify grid display toggle affects all panes.

**Preconditions**:
- At least one pane with oscillator

**User Actions**:
1. Expand Display Options section
2. Toggle "Show Grid" off
3. Observe all panes
4. Toggle "Show Grid" on

**Expected Results**:
- Grid lines disappear from ALL panes when off
- Grid lines appear in ALL panes when on
- Waveforms still visible (only grid affected)
- Zero-line visibility may be affected

**Verification Points**:
- [ ] Grid visibility toggles in all panes simultaneously
- [ ] Waveforms remain visible
- [ ] State persists after save/load

---

### TC-DIS-002: Auto-Scale Toggle

**Description**: Verify auto-scale affects waveform amplitude.

**Preconditions**:
- Oscillator showing quiet audio signal

**User Actions**:
1. Toggle "Auto-Scale" on
2. Observe waveform amplitude
3. Toggle "Auto-Scale" off

**Expected Results**:
- Auto-Scale ON: Waveform amplitude normalized (fills ~80% of pane height)
- Auto-Scale OFF: Waveform amplitude matches actual signal level (may be small for quiet audio)
- Effect visible on all oscillators

**Verification Points**:
- [ ] Waveform size changes with toggle
- [ ] Quiet signals enlarged when auto-scale on
- [ ] True amplitude shown when auto-scale off

---

### TC-DIS-003: Hold Display Toggle

**Description**: Verify hold display freezes waveform.

**Preconditions**:
- Audio playing, waveform updating

**User Actions**:
1. Toggle "Hold Display" on
2. Observe waveform (should freeze)
3. Wait a few seconds
4. Toggle "Hold Display" off

**Expected Results**:
- Hold ON: Waveform freezes at current state
- Waveform does NOT update even as audio continues
- Hold OFF: Waveform resumes updating
- Audio processing unaffected (playback continues)

**Verification Points**:
- [ ] Waveform frozen when hold on
- [ ] Waveform updates when hold off
- [ ] Audio not interrupted

---

## Category 8: Trigger Settings

### TC-TRG-001: Trigger Mode Selection

**Description**: Verify trigger mode options.

**Preconditions**:
- Oscillator with audio source

**User Actions**:
1. Expand Trigger Settings section
2. Select "Free Running" mode
3. Observe waveform
4. Select "Triggered" mode
5. Observe waveform behavior

**Expected Results**:
- Free Running: Waveform continuously scrolls
- Triggered: Waveform attempts to sync to trigger events
- Mode selection persists

**Verification Points**:
- [ ] Mode changes affect waveform behavior
- [ ] Mode selection persists

---

### TC-TRG-002: Trigger Threshold and Edge

**Description**: Configure trigger threshold and edge detection.

**Preconditions**:
- Trigger mode set to "Triggered"
- Audio with clear transients (drums, etc.)

**User Actions**:
1. Adjust Trigger Threshold slider to -20 dBFS
2. Select "Rising" edge
3. Observe waveform stability
4. Select "Falling" edge
5. Observe waveform behavior

**Expected Results**:
- Threshold sets the level at which trigger occurs
- Rising edge: Triggers when signal crosses threshold going up
- Falling edge: Triggers when signal crosses threshold going down
- Waveform should be more stable/synchronized with consistent audio

**Verification Points**:
- [ ] Threshold value displayed in dBFS
- [ ] Edge selection affects trigger behavior
- [ ] Waveform more stable with good trigger settings

---

## Category 9: Status Bar

### TC-STB-001: Status Bar Visibility Toggle

**Description**: Verify status bar can be shown/hidden.

**Preconditions**:
- Status bar visible

**User Actions**:
1. Open Settings popup
2. Toggle "Status Bar" off
3. Observe UI
4. Toggle "Status Bar" on

**Expected Results**:
- Status bar disappears when toggled off
- Main area expands to use freed space
- Status bar reappears when toggled on
- Metrics continue updating when visible

**Verification Points**:
- [ ] Status bar visibility toggles correctly
- [ ] Layout adjusts appropriately
- [ ] State persists

---

### TC-STB-002: Status Bar Metrics Display

**Description**: Verify all status bar metrics are displayed correctly.

**Preconditions**:
- Status bar visible
- Audio playing

**User Actions**:
1. Observe status bar content

**Expected Results**:
- FPS displayed (target ~60 FPS)
- CPU % displayed (plugin CPU usage)
- Memory displayed in MB
- Rendering mode displayed (OpenGL or Software)
- Oscillator count displayed
- Source count displayed
- Values update periodically (every 100-1000ms)

**Verification Points**:
- [ ] All metrics visible and formatted correctly
- [ ] Values are reasonable (FPS ~60, CPU < 100%, etc.)
- [ ] Rendering mode accurate (OpenGL if enabled)
- [ ] Counts match actual oscillator/source numbers

---

## Category 10: State Persistence

### TC-PER-001: Project Save and Load

**Description**: Verify complete state persists across DAW project save/load.

**Preconditions**:
- Complex configuration:
  - 3 oscillators with different colors, modes
  - Custom theme active
  - 2-column layout
  - Sidebar width modified (350px)
  - Specific timing settings (MELODIC, 1/4 note)
  - Display options configured (grid off, auto-scale on)

**User Actions**:
1. Save DAW project
2. Close project
3. Reopen project
4. Open Oscil UI

**Expected Results**:
- All oscillators restored with correct:
  - Colors
  - Processing modes
  - Visibility states
  - Source assignments (if sources available)
- Custom theme active
- 2-column layout restored
- Sidebar width restored (350px)
- Timing settings restored
- Display options restored

**Verification Points**:
- [ ] Oscillator count matches
- [ ] Each oscillator's properties match saved
- [ ] Theme matches
- [ ] Layout matches
- [ ] Sidebar width matches
- [ ] All section settings match

---

### TC-PER-002: Global Preferences Persistence

**Description**: Verify global preferences persist across sessions.

**Preconditions**:
- Custom global preferences set:
  - Default theme set to custom theme
  - Default sidebar width changed

**User Actions**:
1. Close DAW completely
2. Reopen DAW
3. Create new project
4. Add Oscil plugin

**Expected Results**:
- Default theme from preferences applied to new instance
- Default sidebar width from preferences applied
- Global preferences stored in user preferences file, not project

**Verification Points**:
- [ ] New project uses custom defaults
- [ ] Preferences survive DAW restart

---

## Category 11: Sidebar Sections Interaction

### TC-SBR-001: Section Expand/Collapse

**Description**: Verify sidebar sections can be expanded and collapsed.

**Preconditions**:
- All sidebar sections visible

**User Actions**:
1. Click on "Timing" section header
2. Section collapses
3. Click again to expand
4. Repeat for each section

**Expected Results**:
- Clicking section header toggles expanded/collapsed state
- Collapsed section shows only header
- Expanded section shows full content
- Other sections unaffected
- Section states may persist across sessions

**Verification Points**:
- [ ] Each section toggles correctly
- [ ] Content hidden when collapsed
- [ ] Content visible when expanded
- [ ] Smooth animation (if enabled)

---

### TC-SBR-002: Oscillator List Filtering

**Description**: Verify oscillator list filter functionality.

**Preconditions**:
- Multiple oscillators (some visible, some hidden)

**User Actions**:
1. Click filter dropdown in Oscillator List toolbar
2. Select "Visible Only"
3. Observe list
4. Select "Hidden Only"
5. Select "All"

**Expected Results**:
- "Visible Only": Shows only oscillators with visibility=true
- "Hidden Only": Shows only oscillators with visibility=false
- "All": Shows all oscillators
- Counts update in toolbar

**Verification Points**:
- [ ] Correct oscillators shown for each filter
- [ ] Counts accurate

---

## Category 12: Error Handling

### TC-ERR-001: Source Disconnection Recovery

**Description**: Verify graceful handling when source becomes unavailable.

**Preconditions**:
- Oscillator displaying waveform from source on track 2

**User Actions**:
1. Remove plugin from track 2 (source disappears)
2. Observe oscillator behavior
3. Re-add plugin to track 2

**Expected Results**:
- Oscillator shows "No Source" or placeholder
- Waveform goes flat (no data)
- No crash or error dialog
- When source returns, can be reassigned
- Oscillator configuration preserved

**Verification Points**:
- [ ] No crash on source removal
- [ ] Clear indication of missing source
- [ ] Recovery possible when source returns

---

### TC-ERR-002: Invalid Theme Handling

**Description**: Verify handling of corrupted or missing theme.

**Preconditions**:
- Custom theme created

**User Actions**:
1. (Simulate by manually corrupting theme file or deleting it)
2. Reopen plugin

**Expected Results**:
- Plugin falls back to default system theme
- No crash
- Warning may be shown (optional)
- User can create new theme

**Verification Points**:
- [ ] Fallback to default theme
- [ ] No crash on corrupt theme
- [ ] Plugin fully functional

---

## Category 13: Performance

### TC-PFM-001: High Oscillator Count Performance

**Description**: Verify performance with many oscillators.

**Preconditions**:
- Plugin capable of adding multiple oscillators

**User Actions**:
1. Add 20 oscillators
2. Observe FPS in status bar
3. Observe CPU usage

**Expected Results**:
- FPS remains above 30 (ideally near 60)
- CPU usage stays below warning threshold (10%)
- UI remains responsive
- All waveforms update

**Verification Points**:
- [ ] FPS acceptable with high oscillator count
- [ ] CPU stays reasonable
- [ ] UI responsive (no lag in interactions)

---

### TC-PFM-002: Window Resize Performance

**Description**: Verify smooth resizing.

**User Actions**:
1. Drag window corner to resize continuously

**Expected Results**:
- Window resizes smoothly
- No flickering or artifacts
- Components reflow correctly
- FPS may temporarily dip but recovers

**Verification Points**:
- [ ] Smooth resize
- [ ] No visual artifacts
- [ ] Layout correct at all sizes

---

## Category 14: Master Controls

### TC-MC-001: Timebase Slider Adjustment

**Description**: Verify timebase slider controls the display time window.

**Preconditions**:
- Plugin open with at least one oscillator
- Audio playing through source
- Sidebar expanded, Master Controls section visible

**User Actions**:
1. Locate Timebase slider in Master Controls section
2. Note current waveform display (cycles visible)
3. Drag slider to minimum (1 ms)
4. Observe waveform - should show very short time window
5. Drag slider to 100 ms
6. Drag slider to maximum (60000 ms = 60 seconds)

**Expected Results**:
- **1 ms**: Extremely zoomed in, likely less than one cycle of audio visible
- **100 ms**: Moderate zoom, several cycles visible for typical audio
- **60000 ms**: Extremely zoomed out, many seconds of audio compressed
- Waveform updates in real-time as slider moves
- Value display shows current ms value

**Verification Points**:
- [ ] Slider range is 1-60000 ms
- [ ] Waveform time window changes visibly
- [ ] Value display updates during drag
- [ ] Setting persists after save/load

---

### TC-MC-002: Gain Slider Adjustment

**Description**: Verify gain slider amplifies/attenuates waveform display.

**Preconditions**:
- Plugin open with oscillator showing waveform
- Audio at moderate level (not clipping, not silent)

**User Actions**:
1. Locate Gain slider in Master Controls section
2. Note current waveform amplitude
3. Set gain to -60 dB (minimum)
4. Set gain to 0 dB (unity)
5. Set gain to +12 dB (maximum)
6. Verify waveform doesn't clip at high gain with moderate signal

**Expected Results**:
- **-60 dB**: Waveform nearly flat/invisible (heavily attenuated)
- **0 dB**: Normal amplitude (unity gain)
- **+12 dB**: Amplified waveform, larger peaks
- Changes apply to ALL oscillators (master control)

**Verification Points**:
- [ ] Gain range is -60 to +12 dB
- [ ] Waveform amplitude changes visibly
- [ ] Affects all oscillators, not just one
- [ ] Setting persists after save/load

---

## Category 15: Oscillator List Toolbar

### TC-OLT-001: Filter Tabs (All/Visible/Hidden)

**Description**: Verify oscillator list can be filtered by visibility state.

**Preconditions**:
- At least 4 oscillators: 2 visible, 2 hidden
- Sidebar expanded, Oscillators section visible

**User Actions**:
1. Observe filter tabs above oscillator list: "All", "Visible", "Hidden"
2. Click "All" tab - verify all 4 oscillators shown
3. Click "Visible" tab - verify only 2 visible oscillators shown
4. Click "Hidden" tab - verify only 2 hidden oscillators shown
5. While on "Hidden" tab, toggle one hidden oscillator to visible
6. Verify it disappears from the list (now filtered out)

**Expected Results**:
- **All**: Shows all oscillators regardless of visibility
- **Visible**: Shows only oscillators where visibility = true
- **Hidden**: Shows only oscillators where visibility = false
- Filter updates immediately when oscillator visibility changes
- Currently selected tab is highlighted

**Verification Points**:
- [ ] Filter tabs work correctly
- [ ] Count updates when filtering
- [ ] Dynamic update when visibility toggles
- [ ] Selected tab visually distinct

---

### TC-OLT-002: Oscillator Count Display

**Description**: Verify oscillator count updates correctly.

**Preconditions**:
- Multiple oscillators with mixed visibility states

**User Actions**:
1. Note count display (e.g., "3 visible / 5 total")
2. Add a new oscillator
3. Toggle an oscillator's visibility
4. Delete an oscillator

**Expected Results**:
- Count format shows visible and total counts
- Adding oscillator: total increases
- Toggling visibility: visible count changes
- Deleting oscillator: both counts may change

**Verification Points**:
- [ ] Count format is clear
- [ ] Counts update in real-time
- [ ] Counts match actual oscillator states

---

## Category 16: Oscillator Config Popup Details

### TC-OCP-001: Line Width Slider

**Description**: Verify line width affects waveform stroke thickness.

**Preconditions**:
- Oscillator config popup open
- Oscillator visible in pane

**User Actions**:
1. Locate Line Width slider
2. Set to minimum value
3. Observe waveform line thickness
4. Set to maximum value
5. Compare line thickness

**Expected Results**:
- Minimum: Thin, 1px-like line
- Maximum: Thick, bold line (e.g., 4-5px)
- Change visible immediately in pane
- Setting persists after closing popup

**Verification Points**:
- [ ] Line width visibly changes
- [ ] Applies immediately (no need to close popup)
- [ ] Persists after save/load

---

### TC-OCP-002: Vertical Scale Slider

**Description**: Verify vertical scale zooms waveform amplitude display.

**Preconditions**:
- Oscillator with visible waveform
- Config popup open

**User Actions**:
1. Locate Vertical Scale slider
2. Set to minimum (e.g., 0.1x)
3. Observe waveform appears "compressed" vertically
4. Set to maximum (e.g., 10x)
5. Observe waveform appears "stretched" vertically

**Expected Results**:
- Low scale: Waveform appears smaller, more headroom
- High scale: Waveform appears larger, may clip edges
- This is display scaling, not actual gain (doesn't affect audio)

**Verification Points**:
- [ ] Scale visibly affects waveform height
- [ ] Applies to specific oscillator only
- [ ] Persists after save/load

---

### TC-OCP-003: Vertical Offset Slider

**Description**: Verify vertical offset moves waveform up/down.

**Preconditions**:
- Oscillator with visible waveform
- Config popup open

**User Actions**:
1. Locate Vertical Offset slider
2. Set to center (0)
3. Observe waveform centered in pane
4. Move offset negative - waveform moves down
5. Move offset positive - waveform moves up

**Expected Results**:
- Offset 0: Waveform centered on zero line
- Negative offset: Waveform shifted down
- Positive offset: Waveform shifted up
- Useful for separating overlapping waveforms

**Verification Points**:
- [ ] Offset visibly moves waveform position
- [ ] Applies to specific oscillator only
- [ ] Persists after save/load

---

### TC-OCP-004: Name Editing

**Description**: Verify oscillator name can be edited and saves correctly.

**Preconditions**:
- Oscillator config popup open

**User Actions**:
1. Click on name text field at top of popup
2. Clear existing name
3. Type new name: "Lead Vocal"
4. Press Enter or click outside field
5. Close popup
6. Reopen popup for same oscillator

**Expected Results**:
- Text field is editable
- New name displays in popup header
- New name displays in sidebar oscillator list
- Name persists after closing popup
- Name persists after save/load

**Verification Points**:
- [ ] Text field accepts input
- [ ] Name updates in sidebar list
- [ ] Name persists after close
- [ ] Name persists after save/load

---

### TC-OCP-005: Source Selector

**Description**: Verify oscillator source can be changed via config popup.

**Preconditions**:
- At least 2 sources available
- Oscillator config popup open

**User Actions**:
1. Locate Source selector in popup
2. Note current source
3. Click to open source dropdown/list
4. Select a different source
5. Observe waveform changes to show new source's audio

**Expected Results**:
- Source selector shows available sources
- Selecting different source changes oscillator's data
- Waveform in pane updates to show new source
- Change is immediate

**Verification Points**:
- [ ] All available sources listed
- [ ] Selection changes oscillator source
- [ ] Waveform updates immediately
- [ ] Change persists after save/load

---

## Category 17: Keyboard Navigation & Accessibility

### TC-KEY-001: Escape Key Closes Popups

**Description**: Verify Escape key closes popups, modals, and dropdowns.

**Preconditions**:
- Various popups/modals can be opened

**User Actions**:
1. Open oscillator config popup → Press Escape → Verify closes
2. Open theme editor modal → Press Escape → Verify closes
3. Click a dropdown to open → Press Escape → Verify closes
4. Open settings popup → Press Escape → Verify closes

**Expected Results**:
- Escape consistently closes the topmost popup/modal/dropdown
- No data loss (changes are either saved or discarded appropriately)
- Focus returns to previous element

**Verification Points**:
- [ ] Config popup closes on Escape
- [ ] Theme editor closes on Escape
- [ ] Dropdowns close on Escape
- [ ] Settings popup closes on Escape

---

### TC-KEY-002: Tab Focus Trap in Modals

**Description**: Verify Tab key cycles focus within modals only.

**Preconditions**:
- Theme editor modal open

**User Actions**:
1. Press Tab repeatedly
2. Observe focus moves through controls inside modal
3. Continue pressing Tab past last control
4. Focus should wrap to first control in modal
5. Press Shift+Tab - focus should move backwards

**Expected Results**:
- Focus stays within modal (trapped)
- Tab cycles forward through focusable elements
- Shift+Tab cycles backward
- Focus never escapes to elements behind modal

**Verification Points**:
- [ ] Focus trapped inside modal
- [ ] Tab cycles forward correctly
- [ ] Shift+Tab cycles backward
- [ ] All interactive elements reachable

---

### TC-KEY-003: Arrow Keys on Sliders

**Description**: Verify arrow keys adjust slider values.

**Preconditions**:
- Any slider focused (Tab to it or click)

**User Actions**:
1. Focus a slider (e.g., Opacity slider)
2. Press Right/Up arrow - value increases
3. Press Left/Down arrow - value decreases
4. Press Home - value jumps to minimum
5. Press End - value jumps to maximum

**Expected Results**:
- Right/Up: Increases value by one step
- Left/Down: Decreases value by one step
- Home: Sets to minimum
- End: Sets to maximum
- Changes apply immediately (same as mouse drag)

**Verification Points**:
- [ ] Arrow keys adjust value
- [ ] Home/End jump to limits
- [ ] Changes apply in real-time
- [ ] Works on all sliders

---

### TC-KEY-004: Arrow Keys on Dropdowns

**Description**: Verify arrow keys navigate dropdown options.

**Preconditions**:
- Dropdown is open (clicked to expand)

**User Actions**:
1. Open a dropdown (e.g., Pane selector)
2. Press Down arrow - highlight moves down
3. Press Up arrow - highlight moves up
4. Press Enter - selects highlighted option
5. Press Escape - closes without selecting

**Expected Results**:
- Down/Up: Moves highlight through options
- Enter: Selects current highlighted option
- Escape: Closes dropdown, no change
- Arrow keys wrap at boundaries (optional)

**Verification Points**:
- [ ] Arrow keys move highlight
- [ ] Enter selects option
- [ ] Escape cancels
- [ ] Selected value applied correctly

---

### TC-KEY-005: Space/Enter Activates Controls

**Description**: Verify Space and Enter activate buttons, toggles, checkboxes.

**Preconditions**:
- Various controls can be focused via Tab

**User Actions**:
1. Tab to a toggle (e.g., Show Grid)
2. Press Space - toggle activates
3. Tab to a button (e.g., Add Oscillator)
4. Press Enter - button activates
5. Tab to a checkbox
6. Press Space - checkbox toggles

**Expected Results**:
- Space/Enter activate the focused control
- Same effect as clicking with mouse
- Visual feedback shown (toggle state, button press)

**Verification Points**:
- [ ] Toggles respond to Space
- [ ] Buttons respond to Enter
- [ ] Checkboxes respond to Space
- [ ] Visual feedback matches click behavior

---

### TC-KEY-006: Delete Key on Oscillator List Item

**Description**: Verify Delete/Backspace key deletes selected oscillator.

**Preconditions**:
- Oscillator list item selected/focused

**User Actions**:
1. Click to select an oscillator in sidebar list
2. Press Delete or Backspace key
3. Observe confirmation or immediate deletion

**Expected Results**:
- Delete key triggers oscillator deletion
- Confirmation may appear before deletion
- After deletion, oscillator removed from list and pane

**Verification Points**:
- [ ] Delete key recognized on oscillator item
- [ ] Appropriate confirmation (if any)
- [ ] Oscillator actually deleted
- [ ] UI updates correctly

---

## Category 18: Settings Popup

### TC-SET-001: Status Bar Toggle

**Description**: Verify status bar can be shown/hidden via settings.

**Preconditions**:
- Settings popup or dropdown accessible
- Status bar currently visible

**User Actions**:
1. Open Settings popup
2. Locate "Show Status Bar" toggle
3. Toggle OFF
4. Observe status bar hides
5. Toggle ON
6. Observe status bar shows

**Expected Results**:
- Toggle OFF: Status bar disappears, main area expands
- Toggle ON: Status bar reappears at bottom
- Setting persists after close/reopen

**Verification Points**:
- [ ] Toggle controls status bar visibility
- [ ] Main area resizes appropriately
- [ ] Setting persists after save/load

---

### TC-SET-002: Layout Icons

**Description**: Verify layout icons change column layout.

**Preconditions**:
- Settings popup open
- Multiple panes exist

**User Actions**:
1. Locate layout icons (1-column, 2-column, 3-column)
2. Click 2-column icon
3. Observe panes redistribute into 2 columns
4. Click 3-column icon
5. Observe panes redistribute into 3 columns
6. Click 1-column icon
7. Observe panes stack vertically

**Expected Results**:
- Icons visually represent column layout
- Clicking icon changes layout immediately
- Panes redistribute evenly across columns
- Active icon is highlighted

**Verification Points**:
- [ ] Each icon triggers correct layout
- [ ] Panes redistribute correctly
- [ ] Active layout icon highlighted
- [ ] Setting persists after save/load

---

### TC-SET-003: Edit Theme Button

**Description**: Verify Edit Theme button opens theme editor.

**Preconditions**:
- Settings popup open

**User Actions**:
1. Locate "Edit Theme" button
2. Click button
3. Observe theme editor opens
4. Make a change (if on custom theme)
5. Close theme editor
6. Verify settings popup also closed or still accessible

**Expected Results**:
- Button opens Theme Editor modal
- Theme editor shows current theme
- Changes in editor apply to UI
- After closing editor, can continue using plugin

**Verification Points**:
- [ ] Button opens theme editor
- [ ] Correct theme shown in editor
- [ ] Changes apply to UI
- [ ] Can return to normal usage after

---

## Category 19: Source Items

### TC-SRC-003: Source Activity Indicator

**Description**: Verify source activity indicator shows audio flow.

**Preconditions**:
- Source visible in sidebar Sources section
- Audio can be played/stopped

**User Actions**:
1. Observe source item when audio is STOPPED
2. Activity indicator should be dim/inactive
3. Start audio playback
4. Observe activity indicator becomes active (lit, blinking, or animated)
5. Stop audio playback
6. Observe indicator returns to inactive state

**Expected Results**:
- Inactive (no audio): Indicator dim or static
- Active (audio flowing): Indicator lit, possibly pulsing/blinking
- State updates in near real-time

**Verification Points**:
- [ ] Indicator visible on source item
- [ ] State changes with audio playback
- [ ] Updates promptly (within 100-200ms)

---

### TC-SRC-004: Add to Pane Dropdown

**Description**: Verify "Add to Pane" dropdown creates oscillator from source.

**Preconditions**:
- At least 2 panes exist
- Source visible in sidebar

**User Actions**:
1. Locate "Add to Pane" dropdown on source item
2. Click to open dropdown
3. See list of available panes
4. Select "Pane 2"
5. Observe new oscillator created for this source in Pane 2

**Expected Results**:
- Dropdown shows all available panes
- Selecting pane creates new oscillator
- New oscillator uses this source
- New oscillator appears in selected pane
- New oscillator appears in oscillator list

**Verification Points**:
- [ ] All panes listed in dropdown
- [ ] Selection creates oscillator
- [ ] Oscillator uses correct source
- [ ] Oscillator in correct pane
- [ ] Oscillator list updates

---

## Category 20: Scroll & Viewport Behavior

### TC-SCR-001: Sidebar Sections Scrollable

**Description**: Verify sidebar content scrolls when exceeding visible height.

**Preconditions**:
- Sidebar fully expanded
- Many oscillators or expanded sections

**User Actions**:
1. Expand all sidebar sections
2. If content exceeds visible height, scrollbar appears
3. Scroll down using scrollbar or mouse wheel
4. Scroll back up
5. Verify all sections accessible

**Expected Results**:
- Scrollbar appears when content overflows
- Smooth scrolling with mouse wheel
- All content reachable via scroll
- Scroll position maintained during interactions

**Verification Points**:
- [ ] Scrollbar appears when needed
- [ ] Mouse wheel scrolls content
- [ ] All sections accessible
- [ ] No content cut off

---

### TC-SCR-002: Oscillator List Scrollable

**Description**: Verify oscillator list scrolls with many oscillators.

**Preconditions**:
- 10+ oscillators in list

**User Actions**:
1. Observe oscillator list may require scrolling
2. Scroll through list to see all oscillators
3. Click an oscillator at bottom of list
4. Verify scroll position stable or auto-scrolls appropriately

**Expected Results**:
- List scrollable when many items
- All oscillators reachable
- Selection works at any scroll position
- Drag-and-drop works at any scroll position

**Verification Points**:
- [ ] List scrolls smoothly
- [ ] All items clickable
- [ ] Selection works correctly
- [ ] Drag-and-drop still works

---

### TC-SCR-003: Mouse Wheel on Sliders

**Description**: Verify mouse wheel adjusts slider values.

**Preconditions**:
- Slider is hovered (mouse over it)

**User Actions**:
1. Hover mouse over a slider (don't click)
2. Scroll mouse wheel up - value increases
3. Scroll mouse wheel down - value decreases
4. Verify value changes and waveform updates

**Expected Results**:
- Wheel up: Increases slider value
- Wheel down: Decreases slider value
- Changes apply same as dragging
- Works on any slider

**Verification Points**:
- [ ] Mouse wheel adjusts value
- [ ] Direction intuitive (up = more)
- [ ] Works on all sliders
- [ ] Changes apply immediately

---

### TC-SCR-004: Double-Click to Reset Slider

**Description**: Verify double-clicking a slider resets it to default value.

**Preconditions**:
- Any slider visible (e.g., Opacity, Gain, Timebase)
- Slider currently NOT at default value

**User Actions**:
1. Drag a slider away from its default value
2. Note the current value
3. Double-click on the slider
4. Observe value resets to default

**Expected Results**:
- Slider immediately returns to default value
- Visual updates (waveform changes if applicable)
- Single click does NOT reset (only double-click)

**Verification Points**:
- [ ] Double-click resets to default
- [ ] Single-click does not reset
- [ ] Works on all sliders
- [ ] Default value is appropriate for each slider

---

## Category 21: Tooltips

### TC-TIP-001: Tooltip Appearance on Hover

**Description**: Verify tooltips appear after hover delay.

**Preconditions**:
- Plugin UI visible

**User Actions**:
1. Hover mouse over a button (e.g., Add Oscillator)
2. Wait ~500-1000ms without moving
3. Observe tooltip appears
4. Move mouse away
5. Observe tooltip disappears
6. Repeat for sliders, toggles, icons

**Expected Results**:
- Tooltip appears after brief delay (not instant)
- Shows helpful description of control
- Disappears when mouse leaves
- Positioned near control (above, below, or side)

**Verification Points**:
- [ ] Tooltip appears after delay
- [ ] Content is helpful/descriptive
- [ ] Tooltip disappears on mouse leave
- [ ] Position doesn't obscure control

---

### TC-TIP-002: Tooltips Show Keyboard Shortcuts

**Description**: Verify tooltips include keyboard shortcut hints.

**Preconditions**:
- Controls with keyboard shortcuts exist

**User Actions**:
1. Hover over controls that have shortcuts
2. Observe tooltip content
3. Look for keyboard shortcut notation (e.g., "Delete (⌫)")

**Expected Results**:
- Tooltips for controls with shortcuts show the shortcut
- Format is clear (e.g., "Ctrl+S" or "⌘S")
- Helps users discover keyboard navigation

**Verification Points**:
- [ ] Shortcuts shown in tooltips
- [ ] Format is platform-appropriate
- [ ] Shortcuts actually work as described

---

## Category 22: Confirmation Dialogs

### TC-CNF-001: Delete Oscillator Confirmation

**Description**: Verify deletion of oscillator requires confirmation.

**Preconditions**:
- At least one oscillator exists
- Config popup open for that oscillator

**User Actions**:
1. Click Delete button in oscillator config popup
2. Confirmation dialog should appear
3. Click Cancel - oscillator NOT deleted
4. Click Delete again
5. Click Confirm - oscillator IS deleted

**Expected Results**:
- Confirmation dialog appears (if implemented)
- Cancel aborts deletion
- Confirm proceeds with deletion
- OR: No confirmation but Undo available (alternative UX)

**Verification Points**:
- [ ] Confirmation appears before destructive action
- [ ] Cancel preserves oscillator
- [ ] Confirm deletes oscillator
- [ ] UI updates after deletion

---

### TC-CNF-002: Delete Custom Theme Confirmation

**Description**: Verify deletion of custom theme requires confirmation.

**Preconditions**:
- Custom theme exists
- Theme editor open

**User Actions**:
1. Select a custom theme
2. Click Delete Theme button
3. Confirmation should appear
4. Click Cancel - theme preserved
5. Click Delete again
6. Click Confirm - theme deleted

**Expected Results**:
- Cannot delete system themes (button disabled or hidden)
- Custom themes can be deleted
- Confirmation dialog prevents accidents
- After deletion, UI switches to another theme

**Verification Points**:
- [ ] System themes protected
- [ ] Confirmation for custom theme deletion
- [ ] Cancel preserves theme
- [ ] Confirm deletes and switches theme

---

## Test Results Summary Template

| Category | Test ID | Status | Notes |
|----------|---------|--------|-------|
| Initialization | TC-INIT-001 | | |
| Initialization | TC-INIT-002 | | |
| Sources | TC-SRC-001 | | |
| Sources | TC-SRC-002 | | |
| Oscillators | TC-OSC-001 | | |
| Oscillators | TC-OSC-002 | | |
| Oscillators | TC-OSC-003 | | |
| Oscillators | TC-OSC-004 | | |
| Oscillators | TC-OSC-005 | | |
| Oscillators | TC-OSC-006 | | |
| Oscillators | TC-OSC-007 | | |
| Oscillators | TC-OSC-008 | | |
| Oscillators | TC-OSC-009 | | |
| Themes | TC-THM-001 | | |
| Themes | TC-THM-002 | | |
| Themes | TC-THM-003 | | |
| Themes | TC-THM-004 | | |
| Themes | TC-THM-005 | | |
| Themes | TC-THM-006 | | |
| Layout | TC-LAY-001 | | |
| Layout | TC-LAY-002 | | |
| Layout | TC-LAY-003 | | |
| Layout | TC-LAY-004 | | |
| Layout | TC-LAY-005 | | |
| Layout | TC-LAY-006 | | |
| Timing | TC-TIM-001 | | |
| Timing | TC-TIM-002 | | |
| Timing | TC-TIM-003 | | |
| Timing | TC-TIM-004 | | |
| Display | TC-DIS-001 | | |
| Display | TC-DIS-002 | | |
| Display | TC-DIS-003 | | |
| Triggers | TC-TRG-001 | | |
| Triggers | TC-TRG-002 | | |
| Status Bar | TC-STB-001 | | |
| Status Bar | TC-STB-002 | | |
| Persistence | TC-PER-001 | | |
| Persistence | TC-PER-002 | | |
| Sidebar | TC-SBR-001 | | |
| Sidebar | TC-SBR-002 | | |
| Errors | TC-ERR-001 | | |
| Errors | TC-ERR-002 | | |
| Performance | TC-PFM-001 | | |
| Performance | TC-PFM-002 | | |
| Master Controls | TC-MC-001 | | |
| Master Controls | TC-MC-002 | | |
| Oscillator List Toolbar | TC-OLT-001 | | |
| Oscillator List Toolbar | TC-OLT-002 | | |
| Config Popup Details | TC-OCP-001 | | |
| Config Popup Details | TC-OCP-002 | | |
| Config Popup Details | TC-OCP-003 | | |
| Config Popup Details | TC-OCP-004 | | |
| Config Popup Details | TC-OCP-005 | | |
| Keyboard Navigation | TC-KEY-001 | | |
| Keyboard Navigation | TC-KEY-002 | | |
| Keyboard Navigation | TC-KEY-003 | | |
| Keyboard Navigation | TC-KEY-004 | | |
| Keyboard Navigation | TC-KEY-005 | | |
| Keyboard Navigation | TC-KEY-006 | | |
| Settings Popup | TC-SET-001 | | |
| Settings Popup | TC-SET-002 | | |
| Settings Popup | TC-SET-003 | | |
| Source Items | TC-SRC-003 | | |
| Source Items | TC-SRC-004 | | |
| Scroll & Viewport | TC-SCR-001 | | |
| Scroll & Viewport | TC-SCR-002 | | |
| Scroll & Viewport | TC-SCR-003 | | |
| Scroll & Viewport | TC-SCR-004 | | |
| Tooltips | TC-TIP-001 | | |
| Tooltips | TC-TIP-002 | | |
| Confirmation Dialogs | TC-CNF-001 | | |
| Confirmation Dialogs | TC-CNF-002 | | |

**Status Legend**: PASS | FAIL | BLOCKED | NOT RUN
