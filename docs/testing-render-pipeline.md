# Testing the Render Pipeline

This document describes how to test all features of the new render pipeline using the visual preset system.

## How to Use Test Presets

1. Open the Oscillator Config popup by clicking the gear icon on any oscillator
2. Find the "Visual Preset" dropdown
3. Select a test preset to apply its configuration
4. Observe the waveform display for the expected visual effects

## Test Presets Reference

### Post-Processing Effects

#### test_bloom
**What it tests:** Bloom/glow post-processing effect

**Expected visual result:**
- Bright glow/halo surrounding the waveform
- Soft, luminous edges that bleed outward
- The waveform appears to emit light

**Technical settings:**
- Bloom intensity: 2.0 (high)
- Bloom threshold: 0.5 (catches more brightness)
- Blur iterations: 6
- Spread: 1.5

---

#### test_trails
**What it tests:** Trail/persistence effect (oscilloscope afterglow)

**Expected visual result:**
- Ghostly afterimages following waveform movement
- Previous positions fade slowly
- Creates a "smeared" look showing waveform history

**Technical settings:**
- Decay: 0.05 (slow fade = longer trails)
- Trail opacity: 0.9

---

#### test_chromatic
**What it tests:** Chromatic aberration (RGB color separation)

**Expected visual result:**
- Red, green, and blue color fringing at waveform edges
- Rainbow-like separation effect
- Most visible at high-contrast boundaries

**Technical settings:**
- Intensity: 0.015 (high offset for visibility)

---

#### test_scanlines
**What it tests:** CRT scanline effect

**Expected visual result:**
- Horizontal dark lines across the display
- Retro CRT monitor aesthetic
- Subtle phosphor glow between lines

**Technical settings:**
- Intensity: 0.5 (strong lines)
- Density: 2.0 (visible spacing)
- Phosphor glow: enabled

---

#### test_vignette
**What it tests:** Vignette (darkened corners)

**Expected visual result:**
- Darkened edges and corners
- Spotlight/tunnel vision effect
- Focus draws to center of display

**Technical settings:**
- Intensity: 0.8 (strong darkening)
- Softness: 0.4 (moderate edge transition)

---

#### test_grain
**What it tests:** Film grain effect

**Expected visual result:**
- Noisy, animated texture over the display
- Film-like grain that changes every frame
- Adds analog/vintage quality

**Technical settings:**
- Intensity: 0.3 (very visible grain)
- Speed: 24 FPS (film-like animation)

---

#### test_distortion
**What it tests:** Wave distortion effect

**Expected visual result:**
- Wavy, warped display
- Ripple-like deformations that animate
- Underwater/heat-wave aesthetic

**Technical settings:**
- Intensity: 0.03
- Frequency: 6.0 (multiple waves)
- Speed: 2.0 (animated)

---

#### test_color_grade
**What it tests:** Color grading/correction

**Expected visual result:**
- Shifted color palette
- Increased contrast and saturation
- Warm color temperature tint

**Technical settings:**
- Brightness: +0.1
- Contrast: 1.3
- Saturation: 1.5
- Temperature: 0.3 (warm)

---

#### test_all_post
**What it tests:** Multiple post-processing effects combined

**Expected visual result:**
- Cinematic overall look
- Bloom glow with vignette focusing
- Subtle grain and color enhancement
- Trail persistence

**Technical settings:**
- Bloom, trails, vignette, film grain, and color grade all enabled
- Balanced settings for combined effect

---

## Troubleshooting

### Effects not visible
1. Ensure GPU rendering is enabled (check status bar)
2. Make sure the oscillator has an active audio source

### Poor performance
1. Complex presets require more GPU power
2. Disable bloom or reduce iterations for better performance

### Colors look wrong
1. Color grading can shift colors significantly
2. Chromatic aberration adds color fringing (intentional)
3. Reset to "Default" preset to see original colors

---

## Adding Custom Presets

To add new test presets, modify `VisualConfiguration::getPreset()` in:
`src/rendering/VisualConfiguration.cpp`

And add the preset to the list in `getAvailablePresets()` with an ID and display name.
