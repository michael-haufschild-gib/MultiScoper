# Processing Mode Icons Refactoring Plan

## Overview
1. Extend the UI component library with generic path-based icon support
2. Replace text labels on processing mode controls with icons/shortened text
3. Add comprehensive E2E tests

## Current State
- **OscilButton**: Supports text labels and `juce::Image` icons, but NOT path-based vector icons
- **SegmentedButtonBar**: Only supports text buttons via `addButton(label, id, testId)`
- **OscillatorListItem**: Shows 4 mode buttons (Stereo, Mono, Mid, Side) - missing Left/Right
- **OscillatorConfigPopup**: Shows 6 mode buttons with text labels

## Target State

### Generic UI Component Features
- **OscilButton**: Support `juce::Path` icons that render with theme colors
- **SegmentedButtonBar**: Support both text and path-based icon buttons

### Processing Mode Controls
| Mode | Current Label | New Display |
|------|--------------|-------------|
| FullStereo | "Stereo" / "Full Stereo" | ⟗ (two linked circles icon) |
| Mono | "Mono" | ○ (single circle icon) |
| Mid | "Mid" | M |
| Side | "Side" | S |
| Left | (missing in sidebar) | L |
| Right | (missing in sidebar) | R |

## Implementation Phases

### Phase 1: Extend OscilButton with Generic Path Icon Support
**Files:** `include/ui/components/OscilButton.h`, `src/ui/components/OscilButton.cpp`

This is a **generic UI component feature**, not specific to processing modes.

Add to OscilButton:
```cpp
// Path-based icon support (renders with theme colors)
void setIconPath(const juce::Path& path);
void clearIconPath();
bool hasIconPath() const;
```

Implementation details:
- Store `juce::Path iconPath_` member
- In `paintButton()`, if `hasIconPath()`:
  - Scale path to fit within button bounds with padding
  - Stroke/fill using `getTextColour()` for theme consistency
  - Center the path within the button
- Path icons take precedence over image icons and text

### Phase 2: Extend SegmentedButtonBar with Icon Support
**Files:** `include/ui/components/SegmentedButtonBar.h`, `src/ui/components/SegmentedButtonBar.cpp`

Add overload for path-based buttons:
```cpp
// Add a button with a path-based icon (generic feature)
void addButton(const juce::Path& iconPath, int id, const juce::String& testId = {});
```

This reuses OscilButton's new path icon support internally.

### Phase 3: Create ProcessingModeIcons Helper (Application-Specific)
**New file:** `include/ui/components/ProcessingModeIcons.h`

This is **application-specific** - a convenience helper for common audio processing icons.

```cpp
namespace oscil {
namespace ProcessingModeIcons {
    // Two overlapping circles (stereo symbol)
    juce::Path createStereoIcon(float size);

    // Single circle (mono symbol)
    juce::Path createMonoIcon(float size);
}
}
```

Icon designs:
- **Stereo**: Two circles overlapping like a Venn diagram, linked horizontally
- **Mono**: Single centered circle (same diameter as individual stereo circles)

### Phase 4: Update OscillatorListItem
**File:** `src/ui/OscillatorListItem.cpp`

Change from:
```cpp
modeButtons_->addButton("Stereo", static_cast<int>(ProcessingMode::FullStereo));
modeButtons_->addButton("Mono", static_cast<int>(ProcessingMode::Mono));
modeButtons_->addButton("Mid", static_cast<int>(ProcessingMode::Mid));
modeButtons_->addButton("Side", static_cast<int>(ProcessingMode::Side));
```

To:
```cpp
modeButtons_->addButtonWithPath(ProcessingModeIcons::createStereoIcon(14), static_cast<int>(ProcessingMode::FullStereo), "mode_stereo");
modeButtons_->addButtonWithPath(ProcessingModeIcons::createMonoIcon(14), static_cast<int>(ProcessingMode::Mono), "mode_mono");
modeButtons_->addButton("M", static_cast<int>(ProcessingMode::Mid), "mode_mid");
modeButtons_->addButton("S", static_cast<int>(ProcessingMode::Side), "mode_side");
modeButtons_->addButton("L", static_cast<int>(ProcessingMode::Left), "mode_left");
modeButtons_->addButton("R", static_cast<int>(ProcessingMode::Right), "mode_right");
```

Layout adjustment: Change from 4 buttons at 200px to 6 buttons - may need to adjust width or use min button width.

### Phase 5: Update OscillatorConfigPopup
**File:** `src/ui/OscillatorConfigPopup.cpp`

Apply same icon/text changes for consistency.

### Phase 6: E2E Tests
**New file:** `tests/e2e/test_processing_mode.py`

#### Test Categories:

**1. UI Functionality Tests**
- Test all 6 mode buttons are visible when oscillator selected
- Test clicking each button changes the mode
- Test mode selection persists after deselecting/reselecting oscillator
- Test keyboard navigation (arrow keys) through modes

**2. Screenshot Verification Tests**
- Capture screenshot of mode bar with each mode selected
- Verify correct visual appearance (icons for Stereo/Mono, letters for M/S/L/R)
- Verify theme colors are applied consistently

**3. Audio Processing Verification Tests**
Using test harness to inject known audio signals and verify waveform output:

| Mode | Input L | Input R | Expected Output |
|------|---------|---------|-----------------|
| FullStereo | 1.0 | 0.5 | ch1=1.0, ch2=0.5 (stereo) |
| Mono | 1.0 | 0.5 | ch1=0.75 (mono) |
| Mid | 1.0 | 0.5 | ch1=0.75 (mono) |
| Side | 1.0 | 0.5 | ch1=0.25 (mono) |
| Left | 1.0 | 0.5 | ch1=1.0 (mono) |
| Right | 1.0 | 0.5 | ch1=0.5 (mono) |

Test approach:
1. Add oscillator
2. Set processing mode via UI
3. Inject test audio via test harness API
4. Read waveform data via test harness API
5. Verify output matches expected processing

## Files to Modify/Create

### New Files
- `include/ui/components/ProcessingModeIcons.h`
- `src/ui/components/ProcessingModeIcons.cpp`
- `tests/e2e/test_processing_mode.py`

### Modified Files
- `include/ui/components/OscilButton.h`
- `src/ui/components/OscilButton.cpp`
- `include/ui/components/SegmentedButtonBar.h`
- `src/ui/components/SegmentedButtonBar.cpp`
- `src/ui/OscillatorListItem.cpp`
- `src/ui/OscillatorConfigPopup.cpp`
- `CMakeLists.txt` (add new source files)

## Risk Assessment

| Risk | Level | Mitigation |
|------|-------|------------|
| Icon rendering issues | Low | Test with multiple themes |
| Layout breaks with 6 buttons | Medium | Adjust minButtonWidth or bar width |
| Audio processing bugs | Low | SignalProcessor already tested, just verify UI integration |
| Theme color inconsistency | Low | Use existing getTextColour() pattern |

## Verification Checklist
- [ ] Stereo icon displays as two linked circles
- [ ] Mono icon displays as single circle
- [ ] M/S/L/R text buttons visible and correctly themed
- [ ] All 6 modes selectable in OscillatorListItem
- [ ] All 6 modes selectable in OscillatorConfigPopup
- [ ] Mode selection correctly changes oscillator processing
- [ ] Audio processing correct for each mode
- [ ] Screenshot tests pass
- [ ] Existing E2E tests still pass
- [ ] Theme colors applied consistently across all modes
