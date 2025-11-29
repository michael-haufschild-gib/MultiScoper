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

### Particle System

#### test_particles_along
**What it tests:** Particles emitted along waveform path

**Expected visual result:**
- Stream of orange particles following waveform shape
- Particles emit uniformly along the entire waveform
- Additive blending creates glow effect

**Technical settings:**
- Emission mode: AlongWaveform
- Rate: 200 particles/sec
- Color: Orange (0xFFFFAA00)
- Bloom enabled for glow

---

#### test_particles_peaks
**What it tests:** Particles emitted at amplitude peaks

**Expected visual result:**
- Bursts of cyan particles at waveform peaks
- Particles float upward (negative gravity)
- Audio-reactive - louder audio = more particles

**Technical settings:**
- Emission mode: AtPeaks
- Rate: 150 particles/sec
- Color: Cyan (0xFF00FFFF)
- Gravity: -50 (float up)
- Audio reactive with 3x boost

---

#### test_particles_zero
**What it tests:** Particles emitted at zero crossings

**Expected visual result:**
- Magenta particles appearing where waveform crosses center
- Higher randomness creates scattered effect
- Particles mark frequency of zero crossings

**Technical settings:**
- Emission mode: AtZeroCrossings
- Rate: 100 particles/sec
- Color: Magenta (0xFFFF00FF)
- Randomness: 0.8

---

### 3D Shaders

#### test_3d_ribbon
**What it tests:** 3D volumetric ribbon shader

**Expected visual result:**
- Waveform rendered as 3D tube/ribbon
- Lighting creates depth and shadows
- Auto-rotating camera view
- Subtle bloom glow

**Technical settings:**
- Shader: VolumetricRibbon
- Camera distance: 4.0
- Camera angle: 20 degrees pitch
- Auto-rotate: 15 deg/sec

---

#### test_3d_wireframe
**What it tests:** 3D wireframe mesh shader

**Expected visual result:**
- Retro grid/wireframe 3D mesh
- Neon glowing lines
- Auto-rotating camera view
- Scanlines add CRT aesthetic

**Technical settings:**
- Shader: WireframeMesh
- Mesh resolution: 64x24
- High bloom for neon glow
- Scanlines enabled

---

#### test_3d_orbit
**What it tests:** 3D camera orbit controls

**Expected visual result:**
- 3D waveform viewed from angled position
- Faster auto-rotation (25 deg/sec)
- Tests smooth camera movement

**Technical settings:**
- Shader: VolumetricRibbon
- Camera distance: 3.5
- Camera angles: 45/30 degrees
- Auto-rotate: 25 deg/sec

---

### Material Shaders

#### test_glass
**What it tests:** Glass refraction material

**Expected visual result:**
- Transparent glass-like waveform
- Refraction distorts view through material
- Chromatic dispersion at edges
- Environment reflections from "neon" preset

**Technical settings:**
- Shader: GlassRefraction
- Refractive index: 1.5 (glass IOR)
- Fresnel power: 3.0
- Environment map: neon

---

#### test_chrome
**What it tests:** Liquid chrome material

**Expected visual result:**
- Highly reflective metallic surface
- Animated liquid ripples
- Studio environment reflections
- Very low roughness (mirror-like)

**Technical settings:**
- Shader: LiquidChrome
- Reflectivity: 1.0
- Roughness: 0.05
- Environment map: studio

---

#### test_chrome_cyber
**What it tests:** Chrome with cyber environment

**Expected visual result:**
- Liquid metal with neon reflections
- Cyber/synthwave aesthetic
- Bloom adds to neon glow
- Slightly rougher surface

**Technical settings:**
- Shader: LiquidChrome
- Roughness: 0.1
- Environment map: cyber_space
- Bloom enabled

---

### Combined Feature Tests

#### test_full_demo
**What it tests:** Multiple systems working together

**Expected visual result:**
- 3D volumetric ribbon rendering
- Particles bursting at peaks
- Bloom glow effect
- Vignette focus
- Color saturation boost

**Technical settings:**
- Shader: VolumetricRibbon
- Particles at peaks
- Multiple post-processing effects
- Auto-rotating camera

---

#### test_retro_3d
**What it tests:** Retro aesthetic with 3D wireframe

**Expected visual result:**
- Neon wireframe mesh
- Full CRT effect suite
- High bloom for maximum glow
- Chromatic aberration adds color fringing
- Vignette and grain complete the look

**Technical settings:**
- Shader: WireframeMesh
- Bloom: 1.5 intensity
- Scanlines: 0.4 intensity
- Chromatic aberration
- Vignette and film grain

---

## Troubleshooting

### Effects not visible
1. Ensure GPU rendering is enabled (check status bar)
2. Make sure the oscillator has an active audio source
3. Some effects require audio activity (particles at peaks)

### Poor performance
1. Complex presets (test_full_demo, test_retro_3d) require more GPU power
2. Reduce particle counts if frame rate drops
3. Disable bloom or reduce iterations for better performance

### 3D views not rendering
1. 3D shaders require OpenGL 3.2 Core Profile support
2. Check that your graphics drivers are up to date
3. Material shaders need environment map support

### Colors look wrong
1. Color grading can shift colors significantly
2. Chromatic aberration adds color fringing (intentional)
3. Reset to "Default (Clean)" preset to see original colors

---

## Adding Custom Presets

To add new test presets, modify `VisualConfiguration::getPreset()` in:
`src/rendering/VisualConfiguration.cpp`

And add the preset to the list in `getAvailablePresets()` with an ID and display name.
