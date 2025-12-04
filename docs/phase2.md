# Oscil Phase 2: Best-in-Class Oscilloscope Platform

## Executive Summary

Phase 2 transforms Oscil from a powerful multi-instance oscilloscope with impressive visuals into **the ultimate audio analysis and visualization platform** that professionals rely on for precision work AND bedroom producers use to learn and improve their craft.

**Vision**: Every audio engineer and producer should have Oscil open in every session—not because they have to, but because it makes their work faster, more accurate, and more enjoyable.

**Key Differentiators to Maintain**:
- Automatic multi-instance source discovery and deduplication (unique in market)
- 128-oscillator capacity with per-oscillator customization (highest in market)
- GPU-accelerated effects and shaders (best visual quality)
- Lock-free, real-time safe architecture (professional grade)

**Key Gaps to Close**:
- Measurement and analysis tools (cursors, phase correlation, statistics)
- XY/Lissajous phase visualization
- History scrollback and snapshot comparison
- Pitch/cycle detection for stable triggering
- Educational features for learning users

---

## Current Capabilities Summary

### Core Strengths
| Category | Capability | Competitive Position |
|----------|-----------|---------------------|
| Multi-Instance | Automatic source discovery, deduplication, failover | **Industry-leading** |
| Capacity | 128 oscillators, 64 tracks | **Highest** |
| Visuals | 12 effects, 15+ shaders, GPU-accelerated | **Best-in-class** |
| Processing Modes | FullStereo, Mono, Mid, Side, L, R | Standard |
| Timing | TIME mode, MELODIC mode (16 intervals), 4 trigger types | Good |
| Quality | 4 capture presets, adaptive memory budgeting | Sophisticated |

### Current Feature Set
- **Signal Metrics**: RMS, peak, DC offset, stereo correlation (internal only)
- **Per-Oscillator Config**: Color, opacity, line width, visibility, naming, shader
- **Timing Modes**: Free-running (0.1-4000ms) or beat-synced (1/32 to 8 bars)
- **Trigger Modes**: Rising/Falling edge, Level, Manual, None
- **Effects**: Bloom, trails, chromatic aberration, glitch, film grain, scanlines, etc.
- **Layouts**: 1-3 column panes, collapsible sidebar, status bar

---

## Competitive Landscape

### MOscilloscope (MeldaProduction) - FREE
**Strengths**:
- Built-in pitch detector with frequency/note/cents display
- Multi-period display option
- 64-channel surround/ambisonics support
- A-H comparison with morphing
- GPU-accelerated, 100fps capable

**Weaknesses**:
- Single-instance (no multi-track on one screen)
- Complex UI can overwhelm beginners

### PsyScope Pro (fx23) - PAID
**Strengths**:
- 16 multi-track display with 128 instance support
- Full frequency analysis (spectrum, spectrogram, Wigner-Ville)
- VU/LUFS metering with true peaks
- Phase correlation between any two sources
- Wavecycle tracker with note detection
- History scrollback with WAV export
- Configuration sets for source arrangements

**Weaknesses**:
- Windows only
- Complex routing setup
- Overwhelming feature density

### BlueCat Oscilloscope Multi - PAID (~$49, often $9 on sale)
**Strengths**:
- XY/Lissajous dual-channel phase scope
- 4 memory slots for A/B comparison
- Latency compensation for alignment
- Histogram display
- Named curves for identification
- Loop/flow/trigger display modes

**Weaknesses**:
- Limited to 16 channels
- No beat sync
- Dated visual style

### Oszillos Mega Scope (schulz.audio) - PAID
**Strengths**:
- BPM-synced grid from 1/32 to 16 bars
- Elegant, modern UI
- Note detection (beta)
- Precise and shaded draw modes
- Standalone application

**Weaknesses**:
- Limited analysis features
- No phase correlation tools

### s(M)exoscope (Classic) - FREE
**Strengths**:
- Ultra-stable triggering
- Detailed mouse-over statistics
- Simple, focused interface
- Internal trigger speed control

**Weaknesses**:
- Originally 32-bit only (now modernized)
- Single-channel
- No multi-instance

---

## Target User Personas

### 1. Professional Mix/Mastering Engineer
**Goals**: Precision analysis, phase alignment, quick problem identification
**Pain Points**: Phase issues between tracks, timing misalignment, PDC problems
**Must Have**: Measurement cursors, phase correlation, delay readout, latency compensation
**Delighters**: One-click values for track delay, sum/difference views, statistics overlays

### 2. Bedroom Producer / Hobbyist
**Goals**: Learn what's happening in their mix, improve kick/bass relationship
**Pain Points**: Don't know what "good" looks like, overwhelmed by options
**Must Have**: Presets by use case, educational tooltips, simple layouts
**Delighters**: "Kick & Bass" preset, tutorial mode, visual explanations

### 3. Sound Designer / Synth Nerd
**Goals**: See exactly what their patches are doing, stable single-cycle views
**Pain Points**: Jittery displays, can't lock to pitch, variable waveforms
**Must Have**: Pitch detection, cycle locking, persistence/afterglow
**Delighters**: Note grid overlay, cycle statistics, history scrubbing

### 4. Live Performer / Streamer
**Goals**: Impressive visuals synced to music, audience engagement
**Pain Points**: Too much UI, not full-screen friendly, can't control via MIDI
**Must Have**: Performance mode, MIDI control, full-screen
**Delighters**: Video export, audio-reactive visuals, preset switching

---

## Gap Analysis

### Critical Gaps (vs all major competitors)
| Feature | MOscilloscope | PsyScope Pro | BlueCat Multi | Oscil |
|---------|--------------|--------------|---------------|-------|
| Measurement cursors | Limited | Yes | Yes | **NO** |
| XY/Lissajous view | No | Yes | Yes | **NO** |
| History scrollback | No | Yes | Via memory | **NO** |
| Snapshot comparison | A-H slots | Yes | 4 slots | **NO** |
| Pitch detection | Yes | Yes | No | **NO** |
| Phase correlation | No | Yes | Via XY | **NO** |
| Latency compensation | No | No | Yes | **NO** |

### Competitive Advantages to Leverage
| Feature | Oscil | Nearest Competitor |
|---------|-------|-------------------|
| Multi-instance auto-discovery | Yes | PsyScope (manual routing) |
| Automatic deduplication | Yes | None |
| Failover ownership | Yes | None |
| GPU shader effects | 15+ | None comparable |
| Oscillator capacity | 128 | 16 (BlueCat/PsyScope) |
| Per-oscillator shaders | Yes | None |

---

## Feature Requirements (MoSCoW)

### MUST HAVE - Critical for Professional Use

#### M1: Measurement Cursors
Dual time cursors per pane showing:
- Time delta (ms, samples, musical value at current BPM)
- Amplitude at cursor position (linear + dBFS)
- Perfect for: transient alignment, attack time tuning, delay calibration

#### M2: XY/Lissajous Phase Scope
- Any two waveforms can be displayed as XY plot
- Not limited to L/R stereo—any oscillator pair
- Phase rotation control (virtual, doesn't affect audio)
- Real-time correlation update

#### M3: Cross-Track Phase/Delay Measurement
"Align these two" tool:
- Select waveform A & B → compute offset
- Display: "Bass is +3.2 ms ahead of Kick; +37° at 50 Hz"
- Copy-ready value for DAW track delay

#### M4: History & Persistence
- Analog-style afterglow (0-2 seconds configurable)
- History buffer (last N bars in GPU memory)
- Timeline scrubber to review "what just happened"

#### M5: Freeze/Snapshot System
- Freeze current display (Space key)
- Store snapshots (A, B, C, D slots)
- Compare before/after a mix change
- Export snapshot as image

#### M6: Pitch/Cycle Detection
- Detect fundamental frequency per waveform
- Auto-highlight one period
- "Lock to N cycles" display mode
- Stable, jitter-free view for synth editing

#### M7: Phase Correlation Meter
- Per-pair correlation bar (-1 to +1)
- Visible in status bar or per-pane
- Color-coded (red for out-of-phase, green for in-phase)

#### M8: Sum/Difference Waveform
- If multiple waveforms in pane, show computed sum
- A-B difference view to see processor changes
- Essential for phase stacking verification

### SHOULD HAVE - Important Differentiators

#### S1: Latency/PDC Compensation
- Show plugin delay compensation status
- Per-waveform manual offset (ms/samples/musical)
- Correct for mis-reported PDC

#### S2: Statistics Overlay
- RMS/peak envelope on waveform
- Attack time (10%→90%)
- Decay time (90%→10%)
- Clipping indicators
- DC offset warnings

#### S3: Smart Grids & Guides
- Beat grid with measure numbers (beat-synced modes)
- Note/frequency grid for time-domain
- Dynamic labels ("50 Hz = XX ms period")

#### S4: Zoom Presets
- 1 cycle, 2 cycles
- Attack region (first 50ms)
- Bar view (1, 2, 4 bars)
- Custom saved zoom levels

#### S5: Reference Waveform Loading
- Import external audio file
- Display as reference trace
- Compare current signal to reference (transient matching)

#### S6: MIDI Control & Learn
- Space = freeze/unfreeze
- Arrows = navigate panes
- MIDI learn for preset switching
- CC control for visual parameters

#### S7: Instance Manager Panel
- List all active instances (name, track, color)
- Toggle visibility from master
- Rename/recolor from central UI
- Solo/mute particular sources visually

#### S8: Keyboard Shortcuts
- Comprehensive keyboard navigation
- Customizable bindings
- Displayed in tooltips

### COULD HAVE - Delighters

#### C1: Performance Mode
- Full-screen, minimal UI
- Responds to MIDI (preset, color changes)
- Stage visualizer use case
- OBS-friendly window

#### C2: Video/Image Export
- Render current view to video (few bars)
- Export image sequence
- "Scope music" video creation for YouTube

#### C3: Mini FFT Panel
- Small spectrum snapshot per pane
- Shows rough frequency content
- Not a full analyzer (recommend schulz.audio for that)

#### C4: Educational Mode
- Built-in example waveforms (sine, saw, clipped, etc.)
- Pattern-explaining tooltips
- "What changed?" A/B analysis with hints

#### C5: Audio-Reactive Visuals
- Amplitude controls particle emission
- Transients affect glow intensity
- Per-pane "visual intensity" knob

#### C6: Floating Windows
- Detach panes to separate windows
- Multi-monitor support
- Per-monitor DPI handling

#### C7: Spectrogram View (Lite)
- Optional per-pane spectrogram mode
- Not competing with dedicated analyzers
- Basic frequency-over-time view

#### C8: "Lock to Note" Mode
- User picks musical note (e.g., A1)
- Scope auto-adapts zoom to that period
- Overlay expected period grid

### WON'T HAVE - Out of Scope for Phase 2

- Full spectrum analyzer (recommend Oszillos Mega Scope)
- Audio processing/effects (visualization only)
- VST hosting/plugin chaining
- DAW transport control
- Audio recording/export (beyond visual export)
- Surround/Ambisonics beyond stereo (future phase)

---

## Feature Details by Category

### Category 1: Measurement & Analysis

#### 1.1 Measurement Cursors
**Description**: Two draggable vertical cursor lines per pane that measure time and amplitude.

**Requirements**:
- Drag handles at top of waveform area
- Cursor A (solid) and Cursor B (dashed) in contrasting colors
- Readout panel showing:
  - Δt in milliseconds
  - Δt in samples (at current sample rate)
  - Δt in musical value (e.g., "0.68 of a 1/16 at 120 BPM")
  - Amplitude at each cursor (linear peak + dBFS)
- Snap-to-zero-crossing option
- Snap-to-peak option
- Lock/unlock cursor positions
- Copy values to clipboard

**Use Cases**:
- Measuring kick attack time
- Checking reverb pre-delay
- Calibrating sidechain timing
- Tuning compressor attack/release

#### 1.2 Statistics Overlay
**Description**: Per-pane statistical information overlaid on waveform.

**Requirements**:
- Toggle display (off by default)
- Show on-waveform:
  - RMS envelope (lighter trace)
  - Peak envelope (dotted trace)
- Show in corner overlay:
  - Current RMS (dBFS)
  - Current peak (dBFS)
  - Crest factor (peak/RMS ratio)
  - DC offset (if >0.1%)
- Clipping indicator (red flash when samples >0dBFS)
- Attack/decay time estimation:
  - Time from 10%→90% (attack)
  - Time from 90%→10% (decay)
  - Triggered by threshold crossing

#### 1.3 Pitch/Cycle Detection
**Description**: Automatic detection of fundamental frequency with cycle-locked display.

**Requirements**:
- Pitch detection algorithm (autocorrelation or FFT-based)
- Display: Frequency (Hz), Note name, Cents deviation
- "Lock to N cycles" mode (1, 2, 4, 8 cycles)
- Stabilized display (no jitter)
- Min/max frequency limits (configurable)
- Pitch tracking history (last N readings)
- Works with complex waveforms (not just pure tones)

### Category 2: Phase & Alignment Tools

#### 2.1 XY/Lissajous View
**Description**: Plot any two waveforms against each other on X-Y axes.

**Requirements**:
- Dedicated XY pane type (or overlay mode)
- Select source for X-axis (any oscillator)
- Select source for Y-axis (any oscillator)
- Default: L/R stereo of selected oscillator
- Phase rotation control (0-360°, virtual)
- Real-time correlation readout (-1 to +1)
- Grid overlay (circular guides for phase)
- Persistence/afterglow option
- Line color from X source, or configurable

**Visual Reference**:
- Diagonal line (45°) = mono
- Circle = 90° phase shift
- Horizontal line = out of phase

#### 2.2 Cross-Track Delay Measurement
**Description**: Calculate time/phase offset between any two waveforms.

**Requirements**:
- "Compare" mode: select oscillator A and B
- Automatic cross-correlation calculation
- Display results:
  - Time offset (ms, samples)
  - Phase offset at fundamental frequency (degrees)
  - Suggested track delay compensation
- "Copy to clipboard" for DAW delay field
- Visual alignment preview (shift B to match A)
- Works best with transient-rich material

#### 2.3 Phase Correlation Meter
**Description**: Real-time phase correlation display.

**Requirements**:
- Horizontal bar meter (-1 to +1)
- Position in status bar or per-oscillator
- Color gradient: Red (-1) → Yellow (0) → Green (+1)
- Peak hold option
- Selectable measurement window (10ms to 1s)
- Per-pair correlation (not just L/R)

#### 2.4 Phase Flip Preview
**Description**: Virtual phase inversion to preview effect before committing.

**Requirements**:
- Per-oscillator "invert phase" toggle
- Visual only (doesn't affect DAW audio)
- See what summing would look like
- A/B comparison with flip

### Category 3: Visualization & Display

#### 3.1 History & Persistence
**Description**: View past waveforms and scroll through time.

**Requirements**:
- Configurable persistence/afterglow (0-2 seconds)
- History buffer (10-120 seconds configurable)
- Timeline scrubber below waveform
- "Freeze at playhead" sync with DAW
- Memory-efficient circular buffer
- GPU-accelerated playback
- Export visible history range

#### 3.2 Smart Grids
**Description**: Context-aware grid overlays.

**Requirements**:
- Beat grid (beat-synced modes):
  - Major grid lines on beats
  - Minor grid lines on subdivisions
  - Measure numbers
  - Bar markers
- Frequency/period grid (free mode):
  - Note frequency labels
  - Period markers (e.g., "50 Hz period")
- Amplitude grid:
  - dBFS markings
  - Configurable reference level
- Grid opacity control
- Grid color from theme

#### 3.3 Zoom Presets
**Description**: Quick-access zoom levels for common views.

**Requirements**:
- Preset buttons: 1 cycle, 2 cycles, 4 cycles
- Preset buttons: Attack (50ms), 1 bar, 4 bars
- Custom zoom save slots (3-5)
- Smooth animated zoom transitions
- Keyboard shortcuts for presets
- Double-click to reset zoom

#### 3.4 Display Modes
**Description**: Different visualization styles per pane.

**Requirements**:
- Standard: Line waveform
- Filled: Waveform with fill to zero
- Points: Sample-accurate dots
- Bars: Vertical bars per sample
- Envelope: RMS envelope only
- Combined: Any combination

### Category 4: Workflow & Integration

#### 4.1 Snapshot System
**Description**: Store and compare waveform captures.

**Requirements**:
- 4 snapshot slots (A, B, C, D)
- "Capture" button stores current view
- "Compare" mode overlays snapshot on live
- Snapshot metadata:
  - Timestamp
  - Source oscillators
  - Zoom level
  - Notes field
- Export snapshot as PNG
- Load snapshot from file

#### 4.2 Reference Waveform
**Description**: Load external audio as reference trace.

**Requirements**:
- Import WAV/AIFF file
- Display as separate trace (dashed line)
- Align options:
  - Manual offset
  - Auto-align to transient
- Opacity control
- On/off toggle
- Multiple reference slots

#### 4.3 Latency Compensation
**Description**: Align waveforms affected by plugin delay.

**Requirements**:
- Display per-oscillator latency info
- Manual offset field (ms/samples)
- "Auto-detect" using correlation
- PDC status indicator
- Negative offset support
- Per-instance vs per-oscillator offset

#### 4.4 Instance Manager
**Description**: Central control of all plugin instances.

**Requirements**:
- Panel showing all active instances
- Per-instance info:
  - Track name
  - Color
  - Active oscillators
  - Status
- Actions:
  - Rename
  - Recolor
  - Show/hide all oscillators
  - Focus (bring instance window to front)
- Drag to reorder
- Bulk operations

### Category 5: Performance & Creative

#### 5.1 Performance Mode
**Description**: Full-screen display for live use.

**Requirements**:
- Toggle to full-screen (Cmd/Ctrl+F)
- Minimal UI (hide sidebar, status bar)
- Large, centered waveform
- Optional: Show only selected panes
- Quick-switch layouts
- MIDI-triggerable presets
- Low-latency rendering

#### 5.2 MIDI Control
**Description**: External control of plugin parameters.

**Requirements**:
- MIDI Learn mode
- Mappable parameters:
  - Freeze/unfreeze
  - Snapshot capture/recall
  - Preset switching
  - Zoom level
  - Pane selection
- MIDI input selector
- Channel filter
- CC response curves

#### 5.3 Audio-Reactive Visuals
**Description**: Visual parameters driven by audio.

**Requirements**:
- Envelope follower per oscillator
- Mappable targets:
  - Line thickness
  - Glow intensity
  - Particle emission rate
  - Effect amounts
- Sensitivity control
- Attack/release times
- "Intensity" master knob per pane

#### 5.4 Export
**Description**: Export visuals as video/image.

**Requirements**:
- Screenshot (PNG) of current view
- Video export options:
  - Duration (bars or seconds)
  - Resolution (720p, 1080p, 4K)
  - Frame rate (30, 60 fps)
  - Format (MP4, MOV)
- Offline rendering (non-realtime)
- Include audio option
- Batch export multiple panes

### Category 6: Educational Features

#### 6.1 Example Waveforms
**Description**: Built-in reference waveforms for learning.

**Requirements**:
- Library of examples:
  - Pure tones: Sine, Triangle, Square, Saw
  - Modified: PWM, Supersaw, Detuned
  - Processed: Soft clip, Hard clip, Limited
  - Real: Kick, Snare, Bass (royalty-free)
- Load as reference trace
- Side-by-side comparison mode
- "Show me" quick access

#### 6.2 Educational Tooltips
**Description**: Contextual explanations of waveform characteristics.

**Requirements**:
- Toggle "learning mode"
- Pattern detection:
  - "Sharp corners indicate high-frequency content"
  - "Flat tops suggest clipping"
  - "Asymmetry indicates even harmonics"
  - "Phase offset visible between sources"
- Non-intrusive popups
- Link to detailed explanations
- Dismissable per-session

#### 6.3 What Changed Analysis
**Description**: A/B comparison with automated insights.

**Requirements**:
- Capture baseline
- Make change in DAW
- Capture comparison
- Automatic analysis:
  - "Attack became sharper (+15%)"
  - "Sustain level increased (+2dB)"
  - "High-frequency content reduced"
- Visual diff overlay
- Before/after toggle

### Category 7: Presets & Workflows

#### 7.1 Persona-Based Presets
**Description**: Ready-to-use layouts for common tasks.

**Requirements**:
- "Kick & Bass" preset:
  - 2-pane vertical layout
  - Beat grid enabled
  - Phase correlation visible
  - Delay measurement ready
- "Sound Design" preset:
  - Large single pane
  - Cycle locking enabled
  - Mid/side view
  - Persistence high
- "Mastering" preset:
  - Wide bar-length view
  - RMS/peak overlay
  - Clipping indicators
  - DC offset warning
- "Live Performance" preset:
  - Full-screen mode
  - High visual intensity
  - MIDI control enabled

#### 7.2 Custom Layout Save
**Description**: User-defined layout presets.

**Requirements**:
- Save current layout as preset
- Include:
  - Pane arrangement
  - Oscillator assignments
  - Zoom levels
  - Display options
  - Effect settings
- Preset browser with preview
- Import/export presets
- Factory reset option

---

## Technical Considerations

### Architecture Implications

#### New Components Needed
1. **MeasurementEngine**: Cursor calculations, statistics
2. **PhaseAnalyzer**: XY view, correlation, delay detection
3. **HistoryBuffer**: GPU-resident waveform history
4. **SnapshotManager**: Capture and comparison logic
5. **PitchDetector**: Autocorrelation/FFT pitch tracking
6. **MidiController**: Learn mode, mapping storage
7. **ExportEngine**: Video/image rendering pipeline

#### Thread Safety Requirements
- All new analysis runs on UI thread (not audio)
- History buffer uses lock-free SPSC queue
- Snapshot storage is copy-on-capture
- MIDI input processed on dedicated thread

#### Performance Targets
- Maintain 60 FPS with all features enabled
- History buffer: <100MB for 2 minutes at 48kHz
- XY view: <1ms per frame calculation
- Pitch detection: <5ms latency

### GPU Considerations
- XY view uses existing shader infrastructure
- History playback uses texture streaming
- Export uses off-screen framebuffer
- Consider WebGPU for future cross-platform

---

## Success Criteria

### Professional Adoption
- [ ] Phase correlation visible without opening separate plugin
- [ ] Delay measurement accurate to <0.1ms
- [ ] Cursors measure to sample accuracy
- [ ] History captures last 30 seconds minimum

### Beginner Accessibility
- [ ] "Kick & Bass" preset gets user analyzing in <30 seconds
- [ ] Educational tooltips explain common issues
- [ ] Default settings work without configuration

### Creative Use
- [ ] Performance mode fills screen beautifully
- [ ] MIDI control enables live switching
- [ ] Export creates shareable videos

### Competitive Position
- [ ] Feature parity with BlueCat for phase analysis
- [ ] Better multi-instance UX than PsyScope
- [ ] Better visuals than all competitors
- [ ] Educational features unique in market

---

## Implementation Priority Order

### Phase 2.1 - Foundation (Core Analysis)
1. Measurement cursors (M1)
2. Freeze/snapshot system (M5)
3. Phase correlation meter (M7)
4. Statistics overlay (S2)

### Phase 2.2 - Phase Tools
5. XY/Lissajous view (M2)
6. Cross-track delay measurement (M3)
7. Phase flip preview (2.4)
8. Latency compensation (S1)

### Phase 2.3 - Stability & History
9. Pitch/cycle detection (M6)
10. History & persistence (M4)
11. Smart grids (S3)
12. Zoom presets (S4)

### Phase 2.4 - Advanced Features
13. Sum/difference waveform (M8)
14. Reference waveform loading (S5)
15. Instance manager (S7)
16. MIDI control (S6)

### Phase 2.5 - Polish & Delight
17. Keyboard shortcuts (S8)
18. Persona presets (7.1)
19. Performance mode (C1)
20. Educational mode (C4)
21. Export (C2)

---

## Appendix: Competitor Feature Matrix

| Feature | Oscil (Current) | Oscil (Phase 2) | MOscilloscope | PsyScope Pro | BlueCat Multi |
|---------|-----------------|-----------------|---------------|--------------|---------------|
| Multi-instance | Auto-discover | Auto-discover | No | Manual 128 | Manual 16 |
| Deduplication | Yes | Yes | No | No | No |
| Oscillators | 128 | 128 | 1 | 16 | 16 |
| GPU Effects | 12+15 shaders | 12+15 shaders | Basic | No | No |
| Measurement cursors | No | Yes | Limited | Yes | Yes |
| XY/Lissajous | No | Yes | No | Yes | Yes |
| Phase correlation | Internal only | Yes | No | Yes | Via XY |
| Pitch detection | No | Yes | Yes | Yes | No |
| History scrollback | No | Yes | No | Yes | Memory slots |
| Snapshot compare | No | 4 slots | A-H slots | Yes | 4 slots |
| Spectrogram | No | Lite | No | Full | No |
| MIDI control | No | Yes | Yes | No | Yes |
| Video export | No | Yes | No | No | No |
| Educational | No | Yes | Tooltips | No | No |
| Price | TBD | TBD | Free | ~$50 | ~$49 |

---

## References

### Competitor Documentation
- [MOscilloscope Features](https://www.meldaproduction.com/MOscilloscope/features)
- [PsyScope Pro](https://fx23.net/psyscope-pro/)
- [BlueCat Oscilloscope Multi](https://www.bluecataudio.com/Products/Product_OscilloscopeMulti/)
- [Oszillos Mega Scope](https://schulz.audio/products/oszillos-mega-scope/)

### Community Feedback Sources
- [KVR Forum: Best VST Oscilloscope](https://www.kvraudio.com/forum/viewtopic.php?t=508474)
- [Reddit: Oscilloscope Plugins](https://www.reddit.com)
- [Integraudio: Top 6 Oscilloscope Plugins 2025](https://integraudio.com/6-best-oscilloscope-plugin/)

---

*Document Version: 1.0*
*Created: December 2024*
*Based on: Competitive analysis, user persona research, existing feature notes*
