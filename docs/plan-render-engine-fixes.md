# Render Engine Fixes - Implementation Plan

## Overview

This plan addresses 5 critical issues preventing advanced render features from working:
1. Post-processing effect settings not passed
2. EnvironmentMapManager not integrated
3. Material settings not passed to shaders
4. Particle physics parameters not connected
5. Default preset configuration

---

## Issue 1: Post-Processing Effect Settings Not Passed

### Problem
`RenderEngine::applyPostProcessing()` checks if effects are enabled via `config.bloom.enabled`, etc., but never calls `setSettings()` on the effect objects. Effects use default internal values instead of the per-waveform configuration.

### Files to Modify
- `src/rendering/RenderEngine.cpp`

### Implementation Steps

#### Step 1.1: Add settings transfer for BloomEffect
**Location:** `RenderEngine::applyPostProcessing()` around line 641

```cpp
if (config.bloom.enabled)
{
    auto* bloom = dynamic_cast<BloomEffect*>(getEffect("bloom"));
    if (bloom && bloom->isCompiled())
    {
        bloom->setSettings(config.bloom);  // ADD THIS LINE
        Framebuffer* dest = (current == ping) ? pong : ping;
        dest->bind();
        bloom->apply(*context_, current, dest, *fbPool_, deltaTime_);
        dest->unbind();
        current = dest;
    }
}
```

#### Step 1.2: Add settings transfer for TrailsEffect
**Location:** Around line 655

```cpp
if (config.trails.enabled && state.historyFBO && state.historyFBO->isValid())
{
    auto* trails = dynamic_cast<TrailsEffect*>(getEffect("trails"));
    if (trails && trails->isCompiled())
    {
        trails->setSettings(config.trails);  // ADD THIS LINE
        // ... rest of code
    }
}
```

#### Step 1.3: Add settings transfer for ColorGradeEffect
**Location:** Around line 670

```cpp
if (config.colorGrade.enabled)
{
    auto* colorGrade = dynamic_cast<ColorGradeEffect*>(getEffect("color_grade"));
    if (colorGrade && colorGrade->isCompiled())
    {
        colorGrade->setSettings(config.colorGrade);  // ADD THIS LINE
        // ... rest of code
    }
}
```

#### Step 1.4: Add settings transfer for ChromaticAberrationEffect
**Location:** Around line 684

```cpp
if (config.chromaticAberration.enabled)
{
    auto* chromatic = dynamic_cast<ChromaticAberrationEffect*>(getEffect("chromatic_aberration"));
    if (chromatic && chromatic->isCompiled())
    {
        chromatic->setSettings(config.chromaticAberration);  // ADD THIS LINE
        // ... rest of code
    }
}
```

#### Step 1.5: Add settings transfer for ScanlineEffect
**Location:** Around line 698

```cpp
if (config.scanlines.enabled)
{
    auto* scanlines = dynamic_cast<ScanlineEffect*>(getEffect("scanlines"));
    if (scanlines && scanlines->isCompiled())
    {
        scanlines->setSettings(config.scanlines);  // ADD THIS LINE
        // ... rest of code
    }
}
```

#### Step 1.6: Add settings transfer for DistortionEffect
**Location:** Around line 712

```cpp
if (config.distortion.enabled)
{
    auto* distortion = dynamic_cast<DistortionEffect*>(getEffect("distortion"));
    if (distortion && distortion->isCompiled())
    {
        distortion->setSettings(config.distortion);  // ADD THIS LINE
        // ... rest of code
    }
}
```

#### Step 1.7: Add settings transfer for VignetteEffect
**Location:** Around line 726

```cpp
if (config.vignette.enabled)
{
    auto* vignette = dynamic_cast<VignetteEffect*>(getEffect("vignette"));
    if (vignette && vignette->isCompiled())
    {
        vignette->setSettings(config.vignette);  // ADD THIS LINE
        // ... rest of code
    }
}
```

#### Step 1.8: Add settings transfer for FilmGrainEffect
**Location:** Around line 740

```cpp
if (config.filmGrain.enabled)
{
    auto* filmGrain = dynamic_cast<FilmGrainEffect*>(getEffect("film_grain"));
    if (filmGrain && filmGrain->isCompiled())
    {
        filmGrain->setSettings(config.filmGrain);  // ADD THIS LINE
        // ... rest of code
    }
}
```

#### Step 1.9: Add required includes
**Location:** Top of `RenderEngine.cpp`

Ensure all effect headers with setSettings() are included (they already are based on current code).

---

## Issue 2: EnvironmentMapManager Not Integrated

### Problem
Material shaders need environment cubemaps for reflections. `EnvironmentMapManager` exists but is never initialized or used by `RenderEngine`.

### Files to Modify
- `include/rendering/RenderEngine.h`
- `src/rendering/RenderEngine.cpp`

### Implementation Steps

#### Step 2.1: Add EnvironmentMapManager include to RenderEngine.h
**Location:** Top includes section

```cpp
#include "rendering/materials/EnvironmentMapManager.h"
```

#### Step 2.2: Add EnvironmentMapManager member to RenderEngine class
**Location:** Private members section in RenderEngine.h (around line 230)

```cpp
// Environment mapping
std::unique_ptr<EnvironmentMapManager> envMapManager_;
```

#### Step 2.3: Initialize EnvironmentMapManager in RenderEngine::initialize()
**Location:** `RenderEngine::initialize()` after particle system init (around line 143)

```cpp
// Initialize environment map manager
envMapManager_ = std::make_unique<EnvironmentMapManager>();
if (!envMapManager_->initialize(context))
{
    DBG("RenderEngine: Failed to initialize environment map manager");
    // Non-fatal, continue without environment maps
}
else
{
    // Create default environment maps for material shaders
    createDefaultEnvironmentMaps();
}
```

#### Step 2.4: Add createDefaultEnvironmentMaps() helper method
**Location:** Add to RenderEngine.h (public or private section)

```cpp
void createDefaultEnvironmentMaps();
```

**Location:** Add implementation to RenderEngine.cpp

```cpp
void RenderEngine::createDefaultEnvironmentMaps()
{
    if (!envMapManager_)
        return;

    // Create environment maps matching preset IDs used in VisualConfiguration
    envMapManager_->createFromPreset("default_studio", EnvironmentPreset::Studio);
    envMapManager_->createFromPreset("studio", EnvironmentPreset::Studio);
    envMapManager_->createFromPreset("neon", EnvironmentPreset::Neon);
    envMapManager_->createFromPreset("cyber_space", EnvironmentPreset::CyberSpace);
    envMapManager_->createFromPreset("dark", EnvironmentPreset::Dark);
    envMapManager_->createFromPreset("sunset", EnvironmentPreset::Sunset);
    envMapManager_->createFromPreset("abstract", EnvironmentPreset::Abstract);

    DBG("RenderEngine: Created " << 7 << " environment maps");
}
```

#### Step 2.5: Release EnvironmentMapManager in shutdown()
**Location:** `RenderEngine::shutdown()` before releasing other resources

```cpp
// Release environment maps
if (envMapManager_)
{
    envMapManager_->release(*context_);
    envMapManager_.reset();
}
```

#### Step 2.6: Add getEnvironmentMapManager() accessor
**Location:** RenderEngine.h public section

```cpp
EnvironmentMapManager* getEnvironmentMapManager() { return envMapManager_.get(); }
```

---

## Issue 3: Material Settings Not Passed to Shaders

### Problem
When rendering 3D/material shaders, the `config.material` settings are not passed to the shader. Additionally, environment maps need to be bound to material shaders.

### Files to Modify
- `src/rendering/RenderEngine.cpp`
- `include/rendering/materials/MaterialShader.h` (verify interface)

### Implementation Steps

#### Step 3.1: Check MaterialShader interface
**Location:** Read `include/rendering/materials/MaterialShader.h` to verify setMaterial() exists

#### Step 3.2: Add helper to convert MaterialSettings to MaterialProperties
**Location:** Add to `RenderEngine.cpp` or as static method

NOTE: `VisualConfiguration::MaterialSettings` and `MaterialShader::MaterialProperties` have different structures. We need a conversion:

```cpp
MaterialProperties convertMaterialSettings(const MaterialSettings& settings, juce::Colour waveformColor)
{
    MaterialProperties props;

    // Map from config to shader properties
    props.reflectivity = settings.reflectivity;
    props.roughness = settings.roughness;
    props.ior = settings.refractiveIndex;
    props.fresnelPower = settings.fresnelPower;

    // Use tint color as base color
    props.baseColorR = settings.tintColor.getFloatRed();
    props.baseColorG = settings.tintColor.getFloatGreen();
    props.baseColorB = settings.tintColor.getFloatBlue();
    props.baseColorA = settings.tintColor.getFloatAlpha();

    // Material type detection (glass vs chrome)
    // Glass: low metallic, high refraction
    // Chrome: high metallic, no refraction
    if (settings.refractiveIndex > 1.0f && settings.reflectivity < 0.5f)
    {
        props.metallic = 0.0f;  // Dielectric (glass)
        props.refractionStrength = 1.0f;
    }
    else
    {
        props.metallic = 1.0f;  // Metallic (chrome)
        props.refractionStrength = 0.0f;
    }

    props.envMapStrength = settings.useEnvironmentMap ? 1.0f : 0.0f;

    return props;
}
```

#### Step 3.3: Pass material settings to material shaders
**Location:** `RenderEngine::renderWaveformGeometry()` around line 568

Replace current 3D shader handling with:

```cpp
if (is3DShader(config.shaderType))
{
    if (auto* shader3D = dynamic_cast<WaveformShader3D*>(shader))
    {
        if (camera_)
            shader3D->setCamera(*camera_);
        shader3D->setLighting(lightingConfig_);
    }

    // Handle material shaders specifically
    if (isMaterialShader(config.shaderType))
    {
        if (auto* materialShader = dynamic_cast<MaterialShader*>(shader))
        {
            // Convert and set material properties from config
            MaterialProperties matProps = convertMaterialSettings(
                config.material, data.colour);
            materialShader->setMaterial(matProps);

            // Bind environment map if available
            if (envMapManager_ && config.material.useEnvironmentMap)
            {
                GLuint envMap = envMapManager_->getMap(config.material.environmentMapId);
                if (envMap == 0)
                {
                    // Fallback to default map
                    envMap = envMapManager_->getDefaultMap();
                }
                materialShader->setEnvironmentMap(envMap);
            }
        }
    }
}
```

#### Step 3.3: Add MaterialShader include if not present
**Location:** Top of RenderEngine.cpp

```cpp
#include "rendering/materials/MaterialShader.h"
```

---

## Issue 4: Particle Physics Parameters Not Connected

### Problem
The emitter config properly stores `gravity` and `drag` from the VisualConfiguration (verified in RenderEngine.cpp lines 598-611). HOWEVER, `ParticleSystem::update()` calls:

```cpp
pool_.updateAll(deltaTime, gravity_, drag_);  // Uses system-level defaults!
```

The system-level `gravity_` and `drag_` members are never set from the emitter config, so all particles use default physics regardless of preset settings.

### Files to Modify
- `src/rendering/RenderEngine.cpp`

### Implementation Steps

#### Step 4.1: Set particle system physics from first emitter or active config
**Location:** `RenderEngine::renderWaveformParticles()` around line 588

Before calling `particleSystem_->update()`, set the system physics from the current waveform's config:

```cpp
void RenderEngine::renderWaveformParticles(const WaveformRenderData& data, WaveformRenderState& state)
{
    if (!particleSystem_ || !particleSystem_->isInitialized())
        return;

    const auto& config = state.visualConfig.particles;
    if (!config.enabled)
        return;

    // Set particle physics from this waveform's configuration
    // This ensures particles behave according to the preset
    particleSystem_->setGravity(config.gravity);
    particleSystem_->setDrag(config.drag);

    // ... rest of existing code ...
}
```

#### Step 4.2: Alternative - Update particles AFTER setting physics
**Location:** After emitter update in `renderWaveformParticles()`

Move or ensure `particleSystem_->update()` is called AFTER physics are set:

```cpp
// Update emitters with waveform data
for (auto emitterId : state.emitterIds)
{
    particleSystem_->updateEmitter(emitterId, data.channel1, data.bounds, deltaTime_);
}

// IMPORTANT: Update particle physics simulation AFTER setting gravity/drag
// This is typically called from RenderEngine::beginFrame(), verify order
```

#### Step 4.3: Verify call order in beginFrame()
**Location:** `RenderEngine::beginFrame()`

Check that `particleSystem_->update(deltaTime_)` is called. If physics are set per-waveform in `renderWaveformParticles()`, ensure this happens BEFORE the global update, OR move physics update to per-waveform level.

**Note:** The current architecture has a limitation - a single ParticleSystem shared across waveforms means the last waveform's physics settings "win". This is acceptable for MVP but could be enhanced later to support per-emitter physics.

---

## Issue 5: Default Preset Enhancement (Optional)

### Problem
The "default" preset has all effects disabled, which is intentional for a clean baseline. However, we should ensure test presets are easily accessible for validation.

### Files to Modify
- `src/rendering/VisualConfiguration.cpp` (optional enhancement)

### Implementation Steps

#### Step 5.1: Add a "demo" preset with moderate effects (optional)
**Location:** `VisualConfiguration::getPreset()`

```cpp
else if (presetName == "demo")
{
    // Balanced demo with visible but non-overwhelming effects
    config.shaderType = ShaderType::NeonGlow;
    config.bloom.enabled = true;
    config.bloom.intensity = 1.0f;
    config.bloom.threshold = 0.7f;
    config.vignette.enabled = true;
    config.vignette.intensity = 0.3f;
}
```

#### Step 5.2: Add to available presets list
**Location:** `VisualConfiguration::getAvailablePresets()`

```cpp
{"demo", "Demo (Balanced Effects)"},
```

---

## Verification Checklist

After implementing all fixes, verify each feature:

### Post-Processing Effects
- [ ] test_bloom preset shows visible glow
- [ ] test_trails preset shows persistence/ghosting
- [ ] test_chromatic preset shows RGB fringing
- [ ] test_scanlines preset shows CRT lines
- [ ] test_vignette preset shows dark corners
- [ ] test_grain preset shows film noise
- [ ] test_distortion preset shows wave warping
- [ ] test_color_grade preset shows color shifts

### Material Shaders
- [ ] test_chrome preset shows reflective surface
- [ ] test_glass preset shows refraction
- [ ] Environment maps load without errors

### Particle System
- [ ] test_particles_along shows particles following waveform
- [ ] test_particles_peaks shows burst at peaks
- [ ] Gravity setting affects particle motion
- [ ] Drag setting affects particle deceleration

### Combined Features
- [ ] test_full_demo runs without crashes
- [ ] test_retro_3d shows combined 3D + post effects

---

## Implementation Order

1. **Issue 1** - Post-processing settings (highest impact, simplest fix)
2. **Issue 2** - EnvironmentMapManager integration
3. **Issue 3** - Material shader settings
4. **Issue 4** - Particle physics parameters
5. **Issue 5** - Optional demo preset

---

## Estimated Changes

| File | Lines Added | Lines Modified |
|------|-------------|----------------|
| RenderEngine.h | ~15 | ~0 |
| RenderEngine.cpp | ~80 | ~40 |
| VisualConfiguration.cpp | ~15 (optional) | ~0 |

**Total: ~110-150 lines of code changes**

---

## Risk Assessment

| Issue | Complexity | Risk | Notes |
|-------|------------|------|-------|
| Issue 1: Effect Settings | Low | Low | Simple setSettings() calls |
| Issue 2: EnvironmentMapManager | Medium | Medium | New subsystem integration |
| Issue 3: Material Settings | Medium | Low | Conversion function needed |
| Issue 4: Particle Physics | Low | Low | Simple setter calls |
| Issue 5: Demo Preset | Low | Low | Optional enhancement |

---

## Testing Strategy

After each issue is fixed, run these presets to verify:

**Issue 1 verification:**
- `test_bloom` - Look for visible glow halos
- `test_vignette` - Look for dark corners

**Issue 2 + 3 verification:**
- `test_chrome` - Look for reflective metallic surface
- `test_glass` - Look for refraction distortion

**Issue 4 verification:**
- `test_particles_peaks` with `gravity = -50` - Particles should float upward
- Modify preset with `gravity = 100` - Particles should fall down
