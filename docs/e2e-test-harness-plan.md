# E2E Test Harness Plan for Oscil4

## Overview

A standalone application that emulates a DAW environment with 3 tracks, controllable via HTTP API, enabling Claude Code to fully autonomously test all UI features, multitrack functionality, and visual output.

## Architecture

```
OscilTestHarness (Application)
├── TestDAW
│   ├── TestTransport (play/stop/BPM/position)
│   ├── TestTrack[0] ─── OscilPluginProcessor + Editor + AudioGenerator
│   ├── TestTrack[1] ─── OscilPluginProcessor + Editor + AudioGenerator
│   └── TestTrack[2] ─── OscilPluginProcessor + Editor + AudioGenerator
├── TestHttpServer (localhost:8765)
├── TestUIController
├── TestElementRegistry
└── TestScreenshot
```

## Directory Structure

```
test_harness/
├── CMakeLists.txt
├── src/
│   ├── Main.cpp                    # Entry point, starts HTTP server
│   ├── TestDAW.h/cpp               # DAW emulation with tracks
│   ├── TestTrack.h/cpp             # Single track with plugin instance
│   ├── TestAudioGenerator.h/cpp    # Waveform generation
│   ├── TestTransport.h/cpp         # Play/stop/BPM/position
│   ├── TestHttpServer.h/cpp        # HTTP API server
│   ├── TestUIController.h/cpp      # UI interaction layer
│   ├── TestElementRegistry.h/cpp   # Element ID registry
│   ├── TestScreenshot.h/cpp        # Screenshot capture
│   └── TestVerification.h/cpp      # Visual verification helpers
├── include/
│   └── TestableComponent.h         # Mixin for testable UI elements
└── tests/
    └── e2e_scenarios/              # Test scenario documentation
```

## HTTP API Specification

### Transport Control
| Endpoint | Method | Body | Response |
|----------|--------|------|----------|
| `/transport/play` | POST | - | `{"success": true}` |
| `/transport/stop` | POST | - | `{"success": true}` |
| `/transport/setBpm` | POST | `{"bpm": 120}` | `{"success": true}` |
| `/transport/setPosition` | POST | `{"samples": 0}` | `{"success": true}` |
| `/transport/state` | GET | - | `{"playing": true, "bpm": 120, "position": 44100}` |

### Track Audio Control
| Endpoint | Method | Body | Response |
|----------|--------|------|----------|
| `/track/{id}/audio` | POST | `{"waveform": "sine", "frequency": 440, "amplitude": 0.8}` | `{"success": true}` |
| `/track/{id}/burst` | POST | `{"samples": 4410, "waveform": "sine"}` | `{"success": true}` |
| `/track/{id}/info` | GET | - | `{"sourceId": "...", "name": "Track 1", "generating": true}` |
| `/track/{id}/buffer` | GET | - | `{"samples": [...], "channels": 2}` |

Waveform options: `sine`, `square`, `saw`, `triangle`, `noise`, `silence`

### UI Interaction
| Endpoint | Method | Body | Response |
|----------|--------|------|----------|
| `/ui/click` | POST | `{"elementId": "addOscillatorButton"}` | `{"success": true}` |
| `/ui/doubleClick` | POST | `{"elementId": "oscillatorItem_0"}` | `{"success": true}` |
| `/ui/select` | POST | `{"elementId": "columnSelector", "itemId": 2}` | `{"success": true}` |
| `/ui/toggle` | POST | `{"elementId": "showGridToggle", "value": true}` | `{"success": true}` |
| `/ui/slider` | POST | `{"elementId": "gainSlider", "value": -6.0}` | `{"success": true}` |
| `/ui/drag` | POST | `{"from": "pane_0", "to": "pane_1"}` | `{"success": true}` |
| `/ui/state` | GET | - | Full element tree (see below) |

### Screenshot & Verification
| Endpoint | Method | Body | Response |
|----------|--------|------|----------|
| `/screenshot` | POST | `{"path": "/tmp/test.png", "element": "window"}` | `{"success": true, "path": "..."}` |
| `/screenshot` | POST | `{"path": "/tmp/pane.png", "element": "pane_0"}` | `{"success": true}` |
| `/verify/waveformRendering` | POST | `{"paneId": "pane_0"}` | `{"rendering": true, "amplitude": 0.7}` |
| `/verify/themeColor` | POST | `{"element": "background"}` | `{"color": "#1a1a2e"}` |

### State Management
| Endpoint | Method | Body | Response |
|----------|--------|------|----------|
| `/state/reset` | POST | - | `{"success": true}` |
| `/state/save` | POST | `{"path": "/tmp/state.xml"}` | `{"success": true}` |
| `/state/load` | POST | `{"path": "/tmp/state.xml"}` | `{"success": true}` |
| `/state/oscillators` | GET | - | `[{"id": "...", "name": "...", "paneId": "..."}]` |
| `/state/panes` | GET | - | `[{"id": "...", "column": 0, "oscillators": 2}]` |

## UI State Response Format

```json
{
  "elements": {
    "columnSelector": {
      "type": "combobox",
      "value": 1,
      "items": ["1 Column", "2 Columns", "3 Columns"],
      "bounds": {"x": 10, "y": 10, "width": 100, "height": 30}
    },
    "addOscillatorButton": {
      "type": "button",
      "enabled": true,
      "bounds": {"x": 120, "y": 10, "width": 80, "height": 30}
    },
    "oscillatorItem_0": {
      "type": "listItem",
      "selected": true,
      "name": "Oscillator 1"
    },
    "oscillatorItem_0_editButton": {
      "type": "button",
      "visible": true
    },
    "oscillatorItem_0_deleteButton": {
      "type": "button",
      "visible": true
    }
  },
  "panes": [
    {"id": "abc123", "oscillatorCount": 2, "column": 0, "bounds": {...}}
  ],
  "sources": [
    {"id": "src1", "name": "Track 1", "active": true},
    {"id": "src2", "name": "Track 2", "active": true},
    {"id": "src3", "name": "Track 3", "active": false}
  ],
  "sidebar": {
    "collapsed": false,
    "width": 280
  }
}
```

## Element IDs to Register

### Toolbar
- `columnSelector` - Column layout dropdown
- `addOscillatorButton` - Add oscillator button
- `themeSelector` - Theme dropdown
- `sidebarToggleButton` - Sidebar toggle

### Sidebar
- `sourcesSection` - Sources list container
- `sourceItem_{sourceId}` - Individual source item
- `sourceItem_{sourceId}_paneSelector` - Pane dropdown on source
- `oscillatorsSection` - Oscillators list container
- `oscillatorItem_{index}` - Individual oscillator item
- `oscillatorItem_{index}_editButton` - Edit button
- `oscillatorItem_{index}_deleteButton` - Delete button

### Sidebar Sections
- `timingSection` - Timing controls
- `timingModeSelector` - Timing mode dropdown
- `masterControlsSection` - Master controls
- `gainSlider` - Gain slider
- `timebaseSlider` - Timebase slider
- `triggerSection` - Trigger settings
- `triggerModeSelector` - Trigger mode dropdown
- `triggerThresholdSlider` - Threshold slider
- `displaySection` - Display options
- `showGridToggle` - Grid toggle
- `autoScaleToggle` - Auto-scale toggle
- `holdDisplayToggle` - Hold toggle

### Panes
- `pane_{index}` - Pane component
- `pane_{index}_header` - Pane header (drag handle)

### Config Popup
- `configPopup` - The popup overlay
- `configPopup_nameField` - Name input
- `configPopup_colorPicker` - Color picker
- `configPopup_sourceSelector` - Source dropdown
- `configPopup_paneSelector` - Pane dropdown
- `configPopup_modeSelector` - Processing mode dropdown
- `configPopup_deleteButton` - Delete button
- `configPopup_closeButton` - Close button

## Test Scenarios

### Category 1: Basic UI
- [ ] App launches without crash
- [ ] Default pane shows waveform when audio plays
- [ ] Column selector changes layout (1/2/3 columns)
- [ ] Theme selector changes colors
- [ ] Sidebar toggle collapses/expands
- [ ] Sidebar resize handle adjusts width

### Category 2: Oscillator Management
- [ ] Add oscillator button creates new oscillator
- [ ] Oscillator appears in sidebar list
- [ ] Click oscillator selects it
- [ ] Double-click opens config popup
- [ ] Edit button opens config popup
- [ ] Delete button removes oscillator
- [ ] Config popup changes oscillator settings

### Category 3: Multitrack Source Handling
- [ ] Multiple tracks appear in Sources list
- [ ] Assigning source to pane creates oscillator
- [ ] Re-assigning source moves oscillator (no duplicates)
- [ ] Each source shows correct track name
- [ ] Source activity indicator updates

### Category 4: Pane Management
- [ ] Drag pane to reorder works
- [ ] Panes distribute correctly in 1/2/3 columns
- [ ] Full width in 1-column mode
- [ ] Equal width in multi-column mode

### Category 5: Display Options
- [ ] Show grid toggle affects display
- [ ] Auto-scale toggle affects amplitude
- [ ] Hold display freezes waveform
- [ ] Gain slider affects amplitude

### Category 6: Timing & Triggering
- [ ] Timing mode changes capture behavior
- [ ] Host sync respects DAW transport
- [ ] Trigger threshold affects capture

### Category 7: State Persistence
- [ ] Save state preserves oscillators
- [ ] Load state restores configuration
- [ ] Theme persists across restart

## Example Test Session (Claude Workflow)

```bash
# 1. Start test harness in background
./build/OscilTestHarness &
sleep 2

# 2. Verify server is running
curl -s localhost:8765/transport/state

# 3. Configure audio on tracks
curl -X POST localhost:8765/track/1/audio \
  -H "Content-Type: application/json" \
  -d '{"waveform":"sine","frequency":440,"amplitude":0.8}'

curl -X POST localhost:8765/track/2/audio \
  -H "Content-Type: application/json" \
  -d '{"waveform":"square","frequency":880,"amplitude":0.5}'

# 4. Start playback
curl -X POST localhost:8765/transport/play

# 5. Wait for waveform to render
sleep 1

# 6. Take baseline screenshot
curl -X POST localhost:8765/screenshot \
  -H "Content-Type: application/json" \
  -d '{"path":"screenshots/e2e_baseline.png"}'

# 7. Change to 1-column layout
curl -X POST localhost:8765/ui/select \
  -H "Content-Type: application/json" \
  -d '{"elementId":"columnSelector","itemId":1}'

# 8. Wait for layout change
sleep 0.5

# 9. Take screenshot and verify
curl -X POST localhost:8765/screenshot \
  -H "Content-Type: application/json" \
  -d '{"path":"screenshots/e2e_1column.png"}'

# 10. Query state to verify pane width
curl -s localhost:8765/ui/state | jq '.panes[0].bounds.width'

# 11. Test add oscillator
curl -X POST localhost:8765/ui/click \
  -H "Content-Type: application/json" \
  -d '{"elementId":"addOscillatorButton"}'

# 12. Verify oscillator was added
curl -s localhost:8765/state/oscillators | jq 'length'

# 13. Test delete button on oscillator
curl -X POST localhost:8765/ui/click \
  -H "Content-Type: application/json" \
  -d '{"elementId":"oscillatorItem_0_deleteButton"}'

# 14. Stop and cleanup
curl -X POST localhost:8765/transport/stop
```

## Implementation Phases

### Phase 1: Core Infrastructure
- TestDAW class with 3 TestTrack instances
- TestAudioGenerator with sine/square/noise/silence
- TestTransport for play/stop/BPM
- Basic HTTP server (health endpoint only)
- Screenshot capture functionality

### Phase 2: UI Instrumentation
- TestElementRegistry system
- Modify UI components to register testIds
- TestUIController for click/select operations
- GET /ui/state endpoint

### Phase 3: Audio Pipeline
- Wire audio generators to processors
- InstanceRegistry integration
- Audio verification endpoints
- Burst mode for trigger testing

### Phase 4: Full API
- All UI interaction endpoints
- State save/load endpoints
- Verification endpoints
- Error handling and logging

### Phase 5: Test Execution
- Document test scenarios
- Create example test scripts
- Screenshot comparison workflow

## Dependencies

- JUCE framework (already in project)
- HTTP library: cpp-httplib (header-only) or JUCE's built-in URL class
- JSON: nlohmann/json (header-only) or juce::var for JSON handling

## Benefits

1. **Fully autonomous testing** - Claude can run complete test suites without human intervention
2. **Visual verification** - Screenshots capture actual rendering, not just state
3. **Multitrack testing** - Tests the core value proposition of multi-instance support
4. **Reproducible** - Same commands produce same results
5. **CI/CD ready** - Can integrate into automated pipelines
