# Plan: Shader-Based Waveform Rendering

## Overview
Add per-oscillator shader selection for GPU-accelerated waveform rendering with an extensible architecture for adding new shaders.

## Architecture

### Shader System Design
```
include/rendering/
├── ShaderRegistry.h       # Singleton managing shader registration
├── WaveformShader.h       # Base class for all waveform shaders
├── shaders/
│   └── NeonGlowShader.h   # Default shader with neon glow effect
src/rendering/
├── ShaderRegistry.cpp
├── WaveformShader.cpp
├── shaders/
│   └── NeonGlowShader.cpp
```

### Key Components

**ShaderId**: String identifier (e.g., "neon_glow", "classic", "gradient")

**WaveformShader (base class)**:
- Pure virtual `compile()` - compiles vertex/fragment shaders
- Pure virtual `render()` - renders waveform with shader
- Virtual `getDisplayName()` - UI display name
- Virtual `getParameterDescriptors()` - for future per-shader params

**ShaderRegistry (singleton)**:
- `registerShader(ShaderId, std::unique_ptr<WaveformShader>)` - add shaders
- `getShader(ShaderId)` - retrieve shader for rendering
- `getAvailableShaders()` - list for UI dropdowns
- `getDefaultShaderId()` - returns "neon_glow"

## Implementation Tasks

### Phase 1: Data Model Changes

#### Task 1.1: Add ShaderId to Oscillator
**Files**: `include/core/Oscillator.h`, `src/core/Oscillator.cpp`

Add to Oscillator class:
```cpp
// After lineWidth_, verticalScale_, etc.
juce::String shaderId_ = "neon_glow";  // Default shader

// Add getter/setter
[[nodiscard]] juce::String getShaderId() const noexcept { return shaderId_; }
void setShaderId(const juce::String& shaderId) noexcept { shaderId_ = shaderId; }
```

#### Task 1.2: Add Shader to State Persistence
**Files**: `include/core/OscilState.h`, `src/core/OscilState.cpp`

Add to StateIds namespace:
```cpp
static const juce::Identifier ShaderId{ "shaderId" };
```

Update `Oscillator::toValueTree()` and `fromValueTree()` to serialize shaderId.

### Phase 2: Shader Infrastructure

#### Task 2.1: Create ShaderRegistry
**Files**: `include/rendering/ShaderRegistry.h`, `src/rendering/ShaderRegistry.cpp`

```cpp
namespace oscil
{

struct ShaderInfo
{
    juce::String id;
    juce::String displayName;
    juce::String description;
};

class ShaderRegistry
{
public:
    static ShaderRegistry& getInstance();

    void registerShader(std::unique_ptr<WaveformShader> shader);
    WaveformShader* getShader(const juce::String& shaderId);
    std::vector<ShaderInfo> getAvailableShaders() const;
    juce::String getDefaultShaderId() const { return "neon_glow"; }

private:
    ShaderRegistry();
    std::unordered_map<juce::String, std::unique_ptr<WaveformShader>> shaders_;
};

} // namespace oscil
```

#### Task 2.2: Create WaveformShader Base Class
**Files**: `include/rendering/WaveformShader.h`, `src/rendering/WaveformShader.cpp`

```cpp
namespace oscil
{

class WaveformShader
{
public:
    virtual ~WaveformShader() = default;

    // Shader identification
    virtual juce::String getId() const = 0;
    virtual juce::String getDisplayName() const = 0;
    virtual juce::String getDescription() const = 0;

    // Shader lifecycle
    virtual bool compile(juce::OpenGLContext& context) = 0;
    virtual void release() = 0;
    virtual bool isCompiled() const = 0;

    // Rendering
    virtual void render(
        juce::OpenGLContext& context,
        const std::vector<float>& samples,
        juce::Colour colour,
        float opacity,
        float lineWidth,
        juce::Rectangle<float> bounds
    ) = 0;

protected:
    // Utility for derived shaders
    static juce::String loadShaderSource(const char* resourceName);
};

} // namespace oscil
```

#### Task 2.3: Create NeonGlowShader
**Files**: `include/rendering/shaders/NeonGlowShader.h`, `src/rendering/shaders/NeonGlowShader.cpp`

GLSL shaders (embedded as strings):

**Vertex Shader**:
```glsl
#version 330 core
layout(location = 0) in vec2 position;
layout(location = 1) in float intensity;

uniform mat4 projection;
uniform float lineWidth;

out float vIntensity;

void main()
{
    gl_Position = projection * vec4(position, 0.0, 1.0);
    vIntensity = intensity;
}
```

**Fragment Shader**:
```glsl
#version 330 core
in float vIntensity;

uniform vec4 baseColor;
uniform float opacity;
uniform float glowIntensity;

out vec4 fragColor;

void main()
{
    // Core waveform color
    vec3 core = baseColor.rgb;

    // Glow effect - brighter at center, fades out
    float glow = exp(-vIntensity * 2.0) * glowIntensity;
    vec3 glowColor = core * (1.0 + glow * 2.0);

    // Bloom/HDR-like effect
    glowColor = glowColor / (glowColor + vec3(1.0));

    fragColor = vec4(glowColor, opacity * (0.5 + glow));
}
```

Implementation notes:
- Render waveform as triangle strip for anti-aliased lines
- Multiple passes: 1) blur pass for glow, 2) main waveform
- Use uniform for glow intensity (can be animated)

### Phase 3: Rendering Integration

#### Task 3.1: Create ShaderWaveformRenderer
**Files**: `include/rendering/ShaderWaveformRenderer.h`, `src/rendering/ShaderWaveformRenderer.cpp`

Component that handles OpenGL rendering when GPU mode is enabled:
```cpp
class ShaderWaveformRenderer : public juce::Component
{
public:
    void setShader(const juce::String& shaderId);
    void setSamples(const std::vector<float>& samples);
    void setColour(juce::Colour colour);
    void setOpacity(float opacity);
    void setLineWidth(float lineWidth);

    void renderOpenGL(juce::OpenGLContext& context);

private:
    WaveformShader* currentShader_ = nullptr;
    // ... sample data, uniforms
};
```

#### Task 3.2: Integrate with WaveformComponent
**Files**: `include/ui/WaveformComponent.h`, `src/ui/WaveformComponent.cpp`

Modify WaveformComponent to:
1. Accept `setShaderId(juce::String)`
2. Check if GPU rendering is enabled via OscilState
3. If GPU enabled: use ShaderWaveformRenderer
4. If GPU disabled: use existing juce::Path rendering (fallback)

```cpp
// Add to WaveformComponent
void setShaderId(const juce::String& shaderId);

private:
    juce::String shaderId_ = "neon_glow";
    std::unique_ptr<ShaderWaveformRenderer> shaderRenderer_;
    bool useShaderRendering_ = false;  // Set based on GPU mode
```

### Phase 4: UI Integration

#### Task 4.1: Add Shader Dropdown to OscillatorConfigPopup
**Files**: `include/ui/OscillatorConfigPopup.h`, `src/ui/OscillatorConfigPopup.cpp`

Add below color swatches section (~line 134):
```cpp
// Shader section (below color swatches)
std::unique_ptr<juce::Label> shaderLabel_;
std::unique_ptr<OscilDropdown> shaderDropdown_;
juce::String shaderId_ = "neon_glow";

void handleShaderChange();
```

Layout: Insert between colorSwatches_ and lineWidthSlider_ in resized().

Populate dropdown from `ShaderRegistry::getInstance().getAvailableShaders()`.

#### Task 4.2: Add Shader Dropdown to AddOscillatorDialog
**Files**: `include/ui/AddOscillatorDialog.h`, `src/ui/AddOscillatorDialog.cpp`

Add to Result struct:
```cpp
struct Result
{
    SourceId sourceId;
    PaneId paneId;
    bool createNewPane;
    juce::String name;
    juce::Colour color;
    juce::String shaderId;  // NEW
};
```

Add shader dropdown below color section:
```cpp
std::unique_ptr<juce::Label> shaderLabel_;
std::unique_ptr<OscilDropdown> shaderDropdown_;
```

Update DIALOG_HEIGHT to accommodate new control (~+50px).

#### Task 4.3: Wire Shader Changes Through Editor
**Files**: `src/ui/PluginEditor.cpp`

In `onOscillatorConfigChanged()` and `onAddOscillatorResult()`:
- Pass shaderId to WaveformComponent when creating/updating oscillators

### Phase 5: Shader Compilation Integration

#### Task 5.1: Compile Shaders on OpenGL Context Attach
**Files**: `src/ui/PluginEditor.cpp`

When OpenGL context is attached:
```cpp
// In PluginEditor constructor or parentHierarchyChanged
if (openGLContext_.isAttached())
{
    auto& registry = ShaderRegistry::getInstance();
    for (const auto& info : registry.getAvailableShaders())
    {
        auto* shader = registry.getShader(info.id);
        if (shader && !shader->isCompiled())
        {
            shader->compile(openGLContext_);
        }
    }
}
```

### Phase 6: Adding New Shaders (Extension Pattern)

To add a new shader, developers:

1. Create header/cpp in `include/rendering/shaders/` and `src/rendering/shaders/`
2. Inherit from WaveformShader
3. Implement required virtual methods
4. Register in ShaderRegistry initialization:

```cpp
// In ShaderRegistry constructor or init function
void ShaderRegistry::registerBuiltInShaders()
{
    registerShader(std::make_unique<NeonGlowShader>());
    registerShader(std::make_unique<ClassicShader>());    // Future
    registerShader(std::make_unique<GradientShader>());   // Future
}
```

## File Changes Summary

### New Files
| File | Description |
|------|-------------|
| `include/rendering/ShaderRegistry.h` | Shader management singleton |
| `src/rendering/ShaderRegistry.cpp` | ShaderRegistry implementation |
| `include/rendering/WaveformShader.h` | Base shader class |
| `src/rendering/WaveformShader.cpp` | Base shader implementation |
| `include/rendering/ShaderWaveformRenderer.h` | OpenGL rendering component |
| `src/rendering/ShaderWaveformRenderer.cpp` | Renderer implementation |
| `include/rendering/shaders/NeonGlowShader.h` | Neon glow shader |
| `src/rendering/shaders/NeonGlowShader.cpp` | Neon glow implementation |

### Modified Files
| File | Changes |
|------|---------|
| `include/core/Oscillator.h` | Add shaderId_ member |
| `src/core/Oscillator.cpp` | Serialize shaderId |
| `include/core/OscilState.h` | Add ShaderId identifier |
| `include/ui/WaveformComponent.h` | Add setShaderId(), shader rendering |
| `src/ui/WaveformComponent.cpp` | Integrate shader rendering |
| `include/ui/OscillatorConfigPopup.h` | Add shader dropdown |
| `src/ui/OscillatorConfigPopup.cpp` | Shader selection UI |
| `include/ui/AddOscillatorDialog.h` | Add shader to Result, dropdown |
| `src/ui/AddOscillatorDialog.cpp` | Shader selection UI |
| `src/ui/PluginEditor.cpp` | Pass shaderId through, compile shaders |
| `src/ui/PaneComponent.cpp` | Pass shaderId to WaveformComponent |
| `CMakeLists.txt` | Add new source files |

## Testing Strategy

### Unit Tests
- `test_shader_registry.cpp`: Registration, retrieval, defaults
- `test_oscillator.cpp`: Add shaderId serialization tests

### E2E Tests
- `test_shader_selection.py`: Shader dropdown interaction
- `test_shader_persistence.py`: Shader saves/loads with project
- `test_shader_rendering.py`: Visual verification (screenshot comparison)

## Dependencies
- OpenGL 3.3+ (already required by JUCE OpenGL)
- JUCE OpenGL module (already enabled)

## Risks and Mitigations

| Risk | Mitigation |
|------|------------|
| Shader compile failures on some GPUs | Fallback to Path rendering, log errors |
| Performance regression | Profile, batch render calls, use VAOs |
| OpenGL version incompatibility | Detect GL version, use compatible shaders |

## Implementation Order

1. Phase 1: Data model (Oscillator, OscilState)
2. Phase 2: Shader infrastructure (Registry, base class)
3. Phase 3: NeonGlowShader implementation
4. Phase 4: UI integration (dropdowns)
5. Phase 5: Rendering integration (WaveformComponent)
6. Phase 6: Testing and polish
