# Implementation Plan: Rendering Mode Toggle (Software/GPU)

## Overview
Add a toggle switch in the Options section to switch between Software and GPU rendering modes for waveform drawing.

## Requirements Summary
1. Add toggle in Options section (below theme selector)
2. GPU mode: waveforms drawn via JUCE OpenGL shaders
3. Software mode: waveforms drawn via software rendering
4. Mode switch triggers waveform redraw
5. Statusbar updates to show "Software" or "GPU"
6. E2E tests verify functionality
7. E2E tests with screenshots verify GPU mode uses OpenGL

## Technical Approach

### How JUCE OpenGL Works
When `juce::OpenGLContext` is attached to a component (like PluginEditor), all child component `paint()` calls are GPU-accelerated automatically via OpenGL. The actual rendering code in WaveformComponent (using `juce::Path` and `juce::Graphics::strokePath()`) doesn't change - JUCE handles the acceleration transparently.

**Switching modes:**
- **GPU Mode**: `openGLContext_.attachTo(*this)` - enables hardware acceleration
- **Software Mode**: `openGLContext_.detach()` - falls back to CPU rendering

This is already partially implemented (OpenGL attaches at startup when `OSCIL_ENABLE_OPENGL` is defined). We just need runtime control.

## Files to Modify

### 1. State Management
**`include/core/OscilState.h`** (lines ~76-77)
- Add `RenderingMode` state identifier to StateIds namespace

**`src/core/OscilState.cpp`**
- Add getter/setter for rendering mode in OscilState class

### 2. UI: Options Section
**`include/ui/sections/OptionsSection.h`**
- Add `OscilToggle` member for GPU rendering toggle
- Add `renderingModeChanged(bool gpuEnabled)` to Listener interface
- Add state tracking for current mode

**`src/ui/sections/OptionsSection.cpp`**
- Create toggle in `setupComponents()` after theme dropdown
- Wire toggle callback to notify listeners
- Update `resized()` layout
- Update `getPreferredHeight()` calculation

### 3. PluginEditor
**`include/ui/PluginEditor.h`**
- Add `setRenderingMode(RenderingMode mode)` method
- Add `RenderingMode currentRenderingMode_` member

**`src/ui/PluginEditor.cpp`**
- Implement `setRenderingMode()`:
  - Attach/detach OpenGL context based on mode
  - Update status bar
  - Force repaint of all panes
- Update SidebarListenerAdapter to forward `renderingModeChanged()`

### 4. StatusBarComponent
**`src/ui/StatusBarComponent.cpp`**
- Already has `setRenderingMode()` and displays mode
- No changes needed (already functional)

### 5. Test Harness API
**`test_harness/src/TestHttpServer.cpp`**
- Add GET `/state/renderingMode` endpoint to query current mode
- Add POST `/state/renderingMode` endpoint to set mode

### 6. E2E Tests
**`tests/e2e/test_rendering_mode.py`** (new file)
- Test toggle existence and visibility
- Test clicking toggle changes statusbar text
- Take screenshots in each mode
- Verify mode persists after toggle

## Implementation Order

1. **OscilState** - Add RenderingMode state identifier and storage
2. **OptionsSection** - Add toggle UI component and listener callback
3. **PluginEditor** - Implement mode switching logic
4. **SidebarListenerAdapter** - Wire callback
5. **Test Harness** - Add API endpoints
6. **E2E Tests** - Create test file
7. **Build & Test** - Verify everything works

## UI Layout (OptionsSection after changes)

```
+----------------------------------+
| GAIN                             |
| [=====slider=====] 0.0 dB        |
+----------------------------------+
| DISPLAY                          |
| [x] Show Grid                    |
| [x] Auto-Scale                   |
| [ ] Hold                         |
+----------------------------------+
| LAYOUT                           |
| [1 Column          v]            |
+----------------------------------+
| THEME                            |
| [Dark              v]            |
+----------------------------------+
| RENDERING                        |  <- NEW SECTION
| [ ] GPU Acceleration             |  <- NEW TOGGLE
+----------------------------------+
```

## E2E Test Cases

1. **test_rendering_toggle_exists**: Verify toggle is present in Options section
2. **test_toggle_changes_statusbar**: Click toggle, verify statusbar shows "GPU"/"Software"
3. **test_mode_switch_redraws**: Take screenshots before/after, verify waveforms visible in both
4. **test_mode_persists**: Toggle mode, close/reopen editor, verify mode preserved

## Notes

- The toggle will only be functional when `OSCIL_ENABLE_OPENGL` is defined at compile time
- If OpenGL is not available, the toggle should be disabled or hidden
- Default mode should match the compile-time setting (GPU if available, Software otherwise)
