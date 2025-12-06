# Feature Requirements: Visual Preset System

**Status**: Draft
**Priority**: High
**Dependencies**: None
**Related To**: 008_visual_preset_management.md

---

## 1. Overview

Extend the visual preset system to support user-created presets with full access to all rendering engine capabilities. Currently, presets are hardcoded (4 presets in `VisualConfiguration::getPreset()`). This feature adds persistent storage, CRUD operations, and exposes all ~80+ configurable visual parameters.

### 1.1 Current State

- **Location**: `include/rendering/VisualConfiguration.h`, `src/rendering/VisualConfiguration.cpp`
- **Existing Presets**: `default`, `vector_scope`, `string_theory`, `crystalline`
- **Storage**: Hardcoded static map in `getPreset()` function
- **Limitations**: Users cannot create, edit, save, or delete presets

### 1.2 Target State

- User-created presets stored as JSON files
- System presets remain read-only
- Full CRUD support via `VisualPresetManager` class
- Preset organization: System (built-in), User (custom), Favorites (marked)
- Live preview support during editing

---

## 2. Entities

### 2.1 VisualPreset Entity

| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | UUID | Required, unique | Stable preset identifier |
| name | string | 1-64 chars, unique per category | Display name |
| description | string | 0-256 chars | Optional description |
| category | PresetCategory | Required | System, User, or Factory |
| isFavorite | boolean | Required | User-marked as favorite |
| isReadOnly | boolean | Required | True for system presets |
| createdAt | timestamp | Required | ISO 8601 UTC |
| modifiedAt | timestamp | Required | ISO 8601 UTC |
| version | int | >= 1 | Schema version for migration |
| configuration | VisualConfiguration | Required | All visual settings |
| thumbnailData | string | Optional, base64 PNG | Cached preview thumbnail |

### 2.2 PresetCategory Enum

| Value | Description |
|-------|-------------|
| System | Built-in, read-only presets (default, vector_scope, etc.) |
| User | User-created custom presets |
| Factory | Bundled preset packs (future extensibility) |

---

## 3. Complete Configuration Schema

The `VisualConfiguration` struct contains all configurable parameters. Each preset stores a complete configuration.

### 3.1 ShaderType Enum

| Value | Category | Description |
|-------|----------|-------------|
| Basic2D | 2D | Simple glowing waveform |
| NeonGlow | 2D | Bright core with intense colored glow |
| GradientFill | 2D | Solid fill fading to transparent |
| DualOutline | 2D | Double concentric outline |
| PlasmaSine | 2D | Animated procedural plasma fill |
| DigitalGlitch | 2D | Randomized geometric displacement |
| VolumetricRibbon | 3D | 3D tube/ribbon with lighting |
| WireframeMesh | 3D | Retro synthwave wireframe grid |
| VectorFlow | 3D | Flowing data segments |
| StringTheory | 3D | Multiple parallel vibrating strings |
| ElectricFlower | 3D | Organic pulsing with irrational multipliers |
| ElectricFiligree | 3D | Lightning/electricity branching |
| GlassRefraction | Material | Glass with refraction and dispersion |
| LiquidChrome | Material | Flowing liquid metal with reflections |
| Crystalline | Material | Faceted diamond-like crystal |

### 3.2 BlendMode Enum

| Value | OpenGL Equivalent | Effect |
|-------|-------------------|--------|
| Alpha | src*srcAlpha + dst*(1-srcAlpha) | Standard transparency |
| Additive | src*srcAlpha + dst | Glow/brightness stacking |
| Multiply | dst*src | Darken |
| Screen | src + dst*(1-src) | Lighten |

### 3.3 Post-Processing Effects

#### 3.3.1 BloomSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| intensity | float | 0.0 | 2.0 | 1.0 | Glow strength |
| threshold | float | 0.0 | 1.0 | 0.8 | Brightness threshold |
| iterations | int | 2 | 8 | 4 | Blur iterations |
| downsampleSteps | int | 2 | 8 | 6 | Mip levels |
| spread | float | 0.5 | 3.0 | 1.0 | Blur spread multiplier |
| softKnee | float | 0.0 | 1.0 | 0.5 | Threshold transition softness |

#### 3.3.2 RadialBlurSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| amount | float | 0.0 | 0.5 | 0.1 | Zoom amount |
| glow | float | 0.0 | 2.0 | 1.0 | Glow intensity multiplier |
| samples | int | 2 | 8 | 4 | Zoom samples |

#### 3.3.3 TrailSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| decay | float | 0.01 | 0.5 | 0.1 | Fade speed (lower = longer trails) |
| opacity | float | 0.0 | 1.0 | 0.8 | Trail opacity |

#### 3.3.4 ColorGradeSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| brightness | float | -1.0 | 1.0 | 0.0 | Brightness adjustment |
| contrast | float | 0.5 | 2.0 | 1.0 | Contrast multiplier |
| saturation | float | 0.0 | 2.0 | 1.0 | Color saturation |
| temperature | float | -1.0 | 1.0 | 0.0 | Color temperature (-1=cool, 1=warm) |
| tint | float | -1.0 | 1.0 | 0.0 | Tint shift (-1=green, 1=magenta) |
| shadows | ARGB | - | - | 0xFF000000 | Shadow tint color |
| highlights | ARGB | - | - | 0xFFFFFFFF | Highlight tint color |

#### 3.3.5 VignetteSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| intensity | float | 0.0 | 1.0 | 0.5 | Vignette darkness |
| softness | float | 0.0 | 1.0 | 0.5 | Edge softness |
| colour | ARGB | - | - | 0xFF000000 | Vignette color |

#### 3.3.6 FilmGrainSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| intensity | float | 0.0 | 0.5 | 0.1 | Grain strength |
| speed | float | 1.0 | 60.0 | 24.0 | Animation FPS |

#### 3.3.7 ChromaticAberrationSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| intensity | float | 0.0 | 0.02 | 0.005 | RGB channel offset |

#### 3.3.8 ScanlineSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| intensity | float | 0.0 | 1.0 | 0.3 | Line darkness |
| density | float | 0.5 | 4.0 | 2.0 | Lines per pixel |
| phosphorGlow | bool | - | - | true | Subtle inter-line glow |

#### 3.3.9 DistortionSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| intensity | float | 0.0 | 0.5 | 0.0 | Wave distortion amount |
| frequency | float | 1.0 | 20.0 | 4.0 | Wave frequency |
| speed | float | 0.1 | 5.0 | 1.0 | Animation speed |

#### 3.3.10 GlitchSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| intensity | float | 0.0 | 1.0 | 0.5 | Overall glitch strength |
| blockSize | float | 0.01 | 0.2 | 0.05 | Glitch block size |
| lineShift | float | 0.0 | 0.1 | 0.02 | Horizontal displacement |
| colorSeparation | float | 0.0 | 0.05 | 0.01 | RGB shift amount |
| flickerRate | float | 1.0 | 30.0 | 10.0 | Flicker frequency |

#### 3.3.11 TiltShiftSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable effect |
| position | float | 0.0 | 1.0 | 0.5 | Focus center position |
| range | float | 0.0 | 1.0 | 0.3 | In-focus range width |
| blurRadius | float | 0.0 | 10.0 | 2.0 | Blur amount |
| iterations | int | 1 | 8 | 3 | Blur quality |

### 3.4 Particle System Settings

#### 3.4.1 ParticleEmissionMode Enum

| Value | Description |
|-------|-------------|
| AlongWaveform | Uniform emission along waveform path |
| AtPeaks | Emit at amplitude peaks |
| AtZeroCrossings | Emit at zero crossings |
| Continuous | Emit from center continuously |
| Burst | Emit all particles at once |

#### 3.4.2 ParticleBlendMode Enum

| Value | Description |
|-------|-------------|
| Additive | Glow effect (brighten) |
| Alpha | Standard transparency |
| Multiply | Darken |
| Screen | Lighten |

#### 3.4.3 ParticleSettings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable particle system |
| emissionMode | ParticleEmissionMode | - | - | AlongWaveform | Emission pattern |
| emissionRate | float | 1.0 | 1000.0 | 100.0 | Particles per second |
| particleLife | float | 0.1 | 10.0 | 2.0 | Particle lifetime (seconds) |
| particleSize | float | 1.0 | 32.0 | 4.0 | Particle size (pixels) |
| particleColor | ARGB | - | - | 0xFFFFAA00 | Particle color |
| blendMode | ParticleBlendMode | - | - | Additive | Blending mode |
| gravity | float | -100.0 | 100.0 | 0.0 | Gravity strength |
| drag | float | 0.0 | 1.0 | 0.1 | Velocity damping |
| randomness | float | 0.0 | 1.0 | 0.5 | Position/velocity randomization |
| velocityScale | float | 0.0 | 5.0 | 1.0 | Initial velocity multiplier |
| audioReactive | bool | - | - | true | React to audio amplitude |
| audioEmissionBoost | float | 1.0 | 10.0 | 2.0 | Transient emission multiplier |
| textureId | string | 0-64 chars | - | "" | Texture name (empty=circle) |
| textureRows | int | 1 | 16 | 1 | Sprite sheet rows |
| textureCols | int | 1 | 16 | 1 | Sprite sheet columns |
| softParticles | bool | - | - | false | Depth-based edge fading |
| softDepthSensitivity | float | 0.1 | 5.0 | 1.0 | Soft particle depth |
| useTurbulence | bool | - | - | false | Enable turbulence |
| turbulenceStrength | float | 0.0 | 10.0 | 0.0 | Turbulence force |
| turbulenceScale | float | 0.1 | 2.0 | 0.5 | Noise frequency |
| turbulenceSpeed | float | 0.1 | 2.0 | 0.5 | Noise animation speed |

### 3.5 3D Rendering Settings (Settings3D)

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable 3D rendering |
| cameraDistance | float | 1.0 | 20.0 | 5.0 | Camera orbit distance |
| cameraAngleX | float | -90.0 | 90.0 | 15.0 | Camera pitch (degrees) |
| cameraAngleY | float | 0.0 | 360.0 | 0.0 | Camera yaw (degrees) |
| autoRotate | bool | - | - | false | Automatic camera rotation |
| rotateSpeed | float | 0.0 | 60.0 | 10.0 | Rotation speed (deg/sec) |
| meshResolutionX | int | 16 | 512 | 128 | Mesh width divisions |
| meshResolutionZ | int | 8 | 128 | 32 | Mesh depth/history divisions |
| meshScale | float | 0.1 | 5.0 | 1.0 | Overall mesh scale |

### 3.6 Material Settings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| enabled | bool | - | - | false | Enable material properties |
| reflectivity | float | 0.0 | 1.0 | 0.5 | Environment reflection strength |
| refractiveIndex | float | 1.0 | 3.0 | 1.5 | Index of refraction (glass=1.5) |
| fresnelPower | float | 0.5 | 5.0 | 2.0 | Fresnel effect strength |
| tintColor | ARGB | - | - | 0xFFFFFFFF | Material tint |
| roughness | float | 0.0 | 1.0 | 0.1 | Surface roughness |
| useEnvironmentMap | bool | - | - | true | Use environment reflections |
| environmentMapId | string | 0-64 chars | - | "default_studio" | Environment map name |

### 3.7 Lighting Configuration

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| lightDirX | float | -1.0 | 1.0 | 0.5 | Light direction X |
| lightDirY | float | -1.0 | 1.0 | 1.0 | Light direction Y |
| lightDirZ | float | -1.0 | 1.0 | 0.3 | Light direction Z |
| ambientR | float | 0.0 | 1.0 | 0.1 | Ambient red |
| ambientG | float | 0.0 | 1.0 | 0.1 | Ambient green |
| ambientB | float | 0.0 | 1.0 | 0.15 | Ambient blue |
| diffuseR | float | 0.0 | 1.0 | 1.0 | Diffuse red |
| diffuseG | float | 0.0 | 1.0 | 1.0 | Diffuse green |
| diffuseB | float | 0.0 | 1.0 | 1.0 | Diffuse blue |
| specularR | float | 0.0 | 1.0 | 1.0 | Specular red |
| specularG | float | 0.0 | 1.0 | 1.0 | Specular green |
| specularB | float | 0.0 | 1.0 | 1.0 | Specular blue |
| specularPower | float | 1.0 | 256.0 | 32.0 | Shininess exponent |
| specularIntensity | float | 0.0 | 2.0 | 0.5 | Specular strength |

### 3.8 Compositing Settings

| Parameter | Type | Min | Max | Default | Description |
|-----------|------|-----|-----|---------|-------------|
| compositeBlendMode | BlendMode | - | - | Alpha | Final compositing blend |
| compositeOpacity | float | 0.0 | 1.0 | 1.0 | Final opacity |

---

## 4. Storage

### 4.1 File Location

| Platform | Directory |
|----------|-----------|
| macOS | `~/Library/Application Support/Oscil/Presets/` |
| Windows | `%APPDATA%\Oscil\Presets\` |
| Linux | `~/.config/Oscil/Presets/` |

### 4.2 File Structure

```
Presets/
├── system/           # Built-in presets (read-only, shipped with app)
│   ├── default.json
│   ├── vector_scope.json
│   ├── string_theory.json
│   └── crystalline.json
├── user/             # User-created presets
│   ├── my_preset.json
│   └── custom_neon.json
└── metadata.json     # Index and favorites
```

### 4.3 Preset JSON Schema

```json
{
  "schema_version": 1,
  "id": "uuid-string",
  "name": "Preset Name",
  "description": "Optional description",
  "category": "user",
  "is_favorite": false,
  "created_at": "2025-01-15T10:30:00Z",
  "modified_at": "2025-01-15T10:30:00Z",
  "configuration": {
    "shader_type": "NeonGlow",
    "composite_blend_mode": "Additive",
    "composite_opacity": 1.0,
    "bloom": {
      "enabled": true,
      "intensity": 1.5,
      "threshold": 0.3,
      "iterations": 4,
      "downsample_steps": 6,
      "spread": 1.2,
      "soft_knee": 0.5
    },
    "trails": { "enabled": false },
    "color_grade": { "enabled": false },
    "vignette": { "enabled": false },
    "film_grain": { "enabled": false },
    "chromatic_aberration": { "enabled": false },
    "scanlines": { "enabled": false },
    "distortion": { "enabled": false },
    "glitch": { "enabled": false },
    "tilt_shift": { "enabled": false },
    "radial_blur": { "enabled": false },
    "particles": { "enabled": false },
    "settings_3d": { "enabled": false },
    "material": { "enabled": false },
    "lighting": {}
  }
}
```

### 4.4 Metadata JSON Schema

```json
{
  "schema_version": 1,
  "last_updated": "2025-01-15T10:30:00Z",
  "presets": [
    {
      "id": "uuid-string",
      "name": "Preset Name",
      "category": "user",
      "is_favorite": true,
      "file_path": "user/my_preset.json"
    }
  ]
}
```

---

## 5. API Requirements

### 5.1 VisualPresetManager Class

Create new class `VisualPresetManager` in `include/core/VisualPresetManager.h`:

```
class VisualPresetManager {
    // Lifecycle
    void initialize();
    void shutdown();

    // Read operations
    std::vector<VisualPreset> getAllPresets() const;
    std::vector<VisualPreset> getPresetsByCategory(PresetCategory category) const;
    std::vector<VisualPreset> getFavoritePresets() const;
    std::optional<VisualPreset> getPresetById(const juce::String& id) const;
    std::optional<VisualPreset> getPresetByName(const juce::String& name) const;

    // Write operations
    Result<VisualPreset> createPreset(const juce::String& name, const VisualConfiguration& config);
    Result<void> updatePreset(const juce::String& id, const VisualConfiguration& config);
    Result<void> renamePreset(const juce::String& id, const juce::String& newName);
    Result<void> deletePreset(const juce::String& id);
    Result<VisualPreset> duplicatePreset(const juce::String& id, const juce::String& newName);

    // Favorites
    Result<void> setFavorite(const juce::String& id, bool isFavorite);

    // Import/Export
    Result<VisualPreset> importPreset(const juce::File& file);
    Result<void> exportPreset(const juce::String& id, const juce::File& destination);

    // Listeners
    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Listener interface
    class Listener {
        virtual void presetAdded(const VisualPreset& preset) {}
        virtual void presetUpdated(const VisualPreset& preset) {}
        virtual void presetRemoved(const juce::String& presetId) {}
        virtual void presetsReloaded() {}
    };
};
```

### 5.2 Result Type

Use result type for error handling:

```
template<typename T>
struct Result {
    bool success;
    T value;           // Valid only if success=true
    juce::String error; // Error message if success=false
};
```

### 5.3 Integration Points

| Component | Integration |
|-----------|-------------|
| `OscilPluginProcessor` | Owns `VisualPresetManager` instance |
| `AddOscillatorDialog` | Reads preset list for dropdown |
| `OscillatorConfigDialog` | Reads/applies presets |
| `VisualPresetEditor` | Creates/edits presets (new dialog) |
| `RenderEngine` | Receives `VisualConfiguration` to apply |

---

## 6. Validation Rules

### 6.1 Preset Name Validation

| Rule | Constraint |
|------|------------|
| Length | 1-64 characters |
| Characters | Alphanumeric, spaces, hyphens, underscores |
| Uniqueness | Unique within category |
| Reserved | Cannot be "default", "none", "new" |

### 6.2 Configuration Validation

| Rule | Check |
|------|-------|
| ShaderType | Must be valid enum value |
| Numeric ranges | All values within documented min/max |
| Color values | Valid ARGB format (0xAARRGGBB) |
| Enabled dependencies | If material.enabled, shaderType must be material shader |
| 3D dependencies | If settings3D.enabled, shaderType should be 3D or material |

### 6.3 File System Validation

| Rule | Check |
|------|-------|
| File exists | For update/delete operations |
| Write permission | User presets directory is writable |
| JSON valid | Preset file parses as valid JSON |
| Schema version | Compatible with current version |

---

## 7. Migration

### 7.1 System Presets Migration

Move existing hardcoded presets to JSON files:

1. On first launch after update, check if `system/` directory exists
2. If not, create directory and write system preset JSON files
3. System presets are read-only (category: "system", isReadOnly: true)
4. `VisualConfiguration::getPreset()` delegates to `VisualPresetManager`

### 7.2 Backward Compatibility

- If `presetId` in saved oscillator state references unknown preset, fall back to "default"
- Log warning when falling back
- Preserve original `presetId` in state for potential future migration

---

## 8. Default System Presets

### 8.1 Required System Presets

| ID | Name | Shader | Key Features |
|----|------|--------|--------------|
| default | Default | Basic2D | Clean waveform, no effects |
| vector_scope | Vector Scope | Basic2D | Scanlines, bloom, phosphor glow, film grain |
| string_theory | String Theory | StringTheory | 3D, auto-rotate, bloom, particles |
| crystalline | Crystalline | Crystalline | Material shader, high refraction, sparkle particles |

### 8.2 Additional Recommended System Presets

| ID | Name | Shader | Key Features |
|----|------|--------|--------------|
| neon_glow | Neon Glow | NeonGlow | Intense bloom, additive blend |
| synthwave | Synthwave | WireframeMesh | Retro grid, scanlines, chromatic aberration |
| liquid_metal | Liquid Metal | LiquidChrome | Material shader, ripples, reflections |
| electric | Electric | ElectricFiligree | Lightning noise, bloom, trails |
| plasma | Plasma | PlasmaSine | Animated procedural fill, color grade |
| minimal | Minimal | Basic2D | No effects, reduced opacity |

---

## 9. Performance Constraints

| Metric | Constraint |
|--------|------------|
| Preset load time | < 50ms per preset |
| List all presets | < 100ms for 100 presets |
| Save preset | < 100ms |
| Memory per preset | < 10KB in memory |
| Disk per preset | < 20KB JSON file |

---

## 10. Testing Requirements

### 10.1 Unit Tests

- [ ] Preset serialization round-trip (JSON to struct and back)
- [ ] All parameter validations
- [ ] Name uniqueness enforcement
- [ ] Category filtering
- [ ] Favorite toggle persistence

### 10.2 Integration Tests

- [ ] Preset applied to oscillator renders correctly
- [ ] System presets load on first launch
- [ ] User preset persists across app restart
- [ ] Import/export produces identical preset

### 10.3 Edge Cases

- [ ] Empty presets directory
- [ ] Corrupted preset file (graceful skip)
- [ ] Duplicate preset names (reject with error)
- [ ] Delete preset while in use (oscillator falls back to default)

---

## 11. Files to Create/Modify

### 11.1 New Files

| File | Purpose |
|------|---------|
| `include/core/VisualPresetManager.h` | Manager class header |
| `src/core/VisualPresetManager.cpp` | Manager implementation |
| `include/core/VisualPreset.h` | Preset entity struct |
| `resources/presets/system/*.json` | System preset files |

### 11.2 Modified Files

| File | Changes |
|------|---------|
| `include/rendering/VisualConfiguration.h` | Add JSON serialization |
| `src/rendering/VisualConfiguration.cpp` | Delegate getPreset to manager |
| `include/plugin/PluginProcessor.h` | Add VisualPresetManager member |
| `src/plugin/PluginProcessor.cpp` | Initialize manager |
| `CMakeLists.txt` | Add new source files |

---

## 12. Quality Gate Checklist

Before marking this feature complete, verify:

- [ ] All 15 ShaderTypes selectable in presets
- [ ] All 11 post-processing effects configurable
- [ ] All particle settings configurable
- [ ] All 3D settings configurable
- [ ] All material settings configurable
- [ ] All lighting settings configurable
- [ ] System presets load correctly
- [ ] User presets persist across restart
- [ ] JSON schema documented and versioned
- [ ] Error handling for all failure modes
- [ ] Performance constraints met
- [ ] Unit tests passing
