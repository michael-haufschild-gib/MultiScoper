# E2E Test Plan - Post-Processing & Presets

## Objective
Verify the functionality and visual output of the new post-processing effects (Lens Flare, Tilt Shift, Bloom, Film Grain) and the newly added presets.

## Test Setup
1. **Application**: `Oscil Test Harness`
2. **Environment**: macOS (Native)
3. **Renderer**: OpenGL

## Test Cases

### 1. Preset Verification
**Action**: Cycle through the following presets using the Visual Settings menu:
- **Neon**: Verify subtle bloom + lens flare.
- **Cyberpunk**: Verify strong bloom + lens flare + scanlines + grain.
- **Liquid Gold**: Verify material shader + tilt shift blur (top/bottom).
- **Cinematic (NEW)**: Verify desaturated color grade + tilt shift + vignette + film grain.
- **Solar Storm (NEW)**: Verify intense lens flares + distortion.

**Expected Result**: Each preset should drastically change the visual appearance. Transitions should be stable.

### 2. Bloom Quality Check
**Action**:
- Select "Neon" preset.
- Feed a high-frequency sine wave (e.g., 10kHz).
- Observe the bloom around the thin waveform lines.

**Expected Result**:
- Bloom should be smooth and continuous, not "blocky" or pixelated.
- No "fireflies" (flickering pixels) should be visible (thanks to Karis Average).

### 3. Lens Flare Interaction
**Action**:
- Select "Solar Storm" preset.
- Adjust signal amplitude to drive peaks off-screen or near edges.

**Expected Result**:
- Lens flares (ghosts and halos) should appear dynamically based on bright signal peaks.
- Flares should move opposite to the signal source (ghost behavior).
- Chromatic aberration (color splitting) should be visible in the flares.

### 4. Tilt Shift Blur
**Action**:
- Select "Liquid Gold" preset.
- Observe the top and bottom 20% of the waveform display.

**Expected Result**:
- The waveform and grid lines at the top and bottom should be blurry.
- The center should be sharp and in focus.

### 5. Color Grading
**Action**:
- Switch between "Default" and "Cinematic".

**Expected Result**:
- "Cinematic" should look noticeably desaturated (less colorful) and higher contrast (darker blacks, brighter whites) compared to "Default".
- Blacks should remain black (not washed out) due to the linear workflow fix.

## Notes for Tester
- Use the `Test Audio Generator` controls in the harness to change waveforms.
- Use the `Settings` dropdown -> `Visual Settings` to change presets.
- If performance drops significantly, check the console logs for GL errors.
