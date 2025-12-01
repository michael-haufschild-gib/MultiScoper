# Architecture Guide for LLM Coding Agents

**Purpose**: Instructions for where to put code and what patterns to follow in the Oscil codebase.
**Tech Stack**: C++20, JUCE 8.0.5, CMake, GoogleTest 1.17.0, cpp-httplib 0.28.0, nlohmann/json 3.11.3

## Project Overview

Oscil is a multi-track audio oscilloscope VST/AU plugin. All code lives in the `oscil` namespace.

## Where to Put New Code

```
include/                    # Header files (.h)
├── core/                   # Core logic: processors, state, data models
├── dsp/                    # Digital signal processing algorithms
├── rendering/              # OpenGL rendering system
│   ├── shaders/            # 2D waveform shaders (extend WaveformShader)
│   ├── shaders3d/          # 3D waveform shaders (extend WaveformShader3D)
│   ├── effects/            # Post-processing effects (extend PostProcessEffect)
│   ├── materials/          # PBR material shaders (extend MaterialShader)
│   └── particles/          # Particle system components
├── ui/                     # UI components (juce::Component subclasses)
│   ├── components/         # Reusable UI component library (buttons, sliders, etc.)
│   ├── sections/           # Sidebar collapsible sections
│   ├── coordinators/       # State-UI coordination logic
│   └── managers/           # Lifecycle managers (OpenGL, etc.)
└── Oscil.h                 # Main umbrella header

src/                        # Implementation files (.cpp)
├── core/                   # Core implementations
├── dsp/                    # DSP implementations
├── rendering/              # GPU rendering implementations
│   ├── shaders/            # 2D shader implementations
│   ├── shaders3d/          # 3D shader implementations
│   ├── effects/            # Post-process effect implementations
│   ├── materials/          # Material shader implementations
│   └── particles/          # Particle system implementations
└── ui/                     # UI implementations
    ├── components/         # Reusable UI component implementations
    ├── sections/           # Sidebar section implementations
    ├── coordinators/       # Coordinator implementations
    └── managers/           # Manager implementations

tests/                      # GoogleTest unit tests
└── test_*.cpp              # Test files

test_harness/               # E2E test harness (separate JUCE app)
├── include/                # Test harness headers
└── src/                    # Test harness implementations
```

**Decision tree - where does my code go?**
- Audio processing logic? → `src/dsp/` + `include/dsp/`
- State management/data models? → `src/core/` + `include/core/`
- 2D waveform shader? → `src/rendering/shaders/` + `include/rendering/shaders/`
- 3D waveform shader? → `src/rendering/shaders3d/` + `include/rendering/shaders3d/`
- Post-processing effect? → `src/rendering/effects/` + `include/rendering/effects/`
- PBR material shader? → `src/rendering/materials/` + `include/rendering/materials/`
- Particle system? → `src/rendering/particles/` + `include/rendering/particles/`
- GPU rendering infrastructure? → `src/rendering/` + `include/rendering/`
- UI rendering/components? → `src/ui/` + `include/ui/`
- Collapsible sidebar section? → `src/ui/sections/` + `include/ui/sections/`
- State-UI coordination? → `src/ui/coordinators/` + `include/ui/coordinators/`
- Lifecycle management (OpenGL, etc.)? → `src/ui/managers/` + `include/ui/managers/`
- Unit test? → `tests/test_[class_name].cpp`
- E2E test harness code? → `test_harness/src/` + `test_harness/include/`

## File Naming Conventions

| Type | Location | Naming Pattern | Example |
|------|----------|----------------|---------|
| Header | `include/[domain]/` | `PascalCase.h` | `WaveformComponent.h` |
| Implementation | `src/[domain]/` | `PascalCase.cpp` | `WaveformComponent.cpp` |
| Test | `tests/` | `test_snake_case.cpp` | `test_signal_processor.cpp` |

## Class Naming Conventions

- Classes: `PascalCase` (e.g., `WaveformComponent`, `SignalProcessor`)
- Member variables: `camelCase_` with trailing underscore (e.g., `captureBuffer_`)
- Methods: `camelCase` (e.g., `getCaptureBuffer()`)
- Enums: `PascalCase` for type, `PascalCase` for values (e.g., `ProcessingMode::FullStereo`)

## How to Create a New UI Component

**Step 1**: Create header at `include/ui/MyComponent.h`

```cpp
/*
    Oscil - My Component Header
    Brief description of what it does
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

namespace oscil
{

class MyComponent : public juce::Component
{
public:
    MyComponent();
    ~MyComponent() override = default;

    void paint(juce::Graphics& g) override;
    void resized() override;

private:
    // Member variables with trailing underscore
    float someValue_ = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MyComponent)
};

} // namespace oscil
```

**Step 2**: Create implementation at `src/ui/MyComponent.cpp`

```cpp
/*
    Oscil - My Component
    Brief description
*/

#include "ui/MyComponent.h"

namespace oscil
{

MyComponent::MyComponent()
{
    // Constructor implementation
}

void MyComponent::paint(juce::Graphics& g)
{
    // Rendering
}

void MyComponent::resized()
{
    // Layout
}

} // namespace oscil
```

**Step 3**: Add to `CMakeLists.txt` under `target_sources(Oscil PRIVATE ...)`

```cmake
src/ui/MyComponent.cpp
```

## How to Create a New DSP Class

**Step 1**: Create header at `include/dsp/MyProcessor.h`

```cpp
/*
    Oscil - My Processor Header
    Brief description
*/

#pragma once

#include <vector>

namespace oscil
{

class MyProcessor
{
public:
    MyProcessor();

    void process(const float* input, float* output, int numSamples);

private:
    std::vector<float> buffer_;
};

} // namespace oscil
```

**Step 2**: Create implementation at `src/dsp/MyProcessor.cpp`

**Step 3**: Add to `CMakeLists.txt`

## How to Create a New Core Class

**Step 1**: Create header at `include/core/MyClass.h`
**Step 2**: Create implementation at `src/core/MyClass.cpp`
**Step 3**: Add to `CMakeLists.txt`

## How to Add a Processing Mode

1. Add enum value to `ProcessingMode` in `include/dsp/SignalProcessor.h`:
```cpp
enum class ProcessingMode
{
    FullStereo,
    Mono,
    // ... existing modes
    MyNewMode  // Add here
};
```

2. Implement processing in `SignalProcessor::process()` in `src/dsp/SignalProcessor.cpp`:
```cpp
case ProcessingMode::MyNewMode:
    // Implement your processing logic
    break;
```

3. Add string conversion if needed
4. Update UI selectors

## How to Create a Sidebar Section

Sidebar sections are collapsible UI panels in the sidebar.

**Step 1**: Create header at `include/ui/sections/MySidebarSection.h`

```cpp
/*
    Oscil - My Sidebar Section
    Description of what this section controls
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/ThemeManager.h"
#include "ui/sections/SectionConstants.h"

namespace oscil
{

class MySidebarSection : public juce::Component,
                         public ThemeManagerListener
{
public:
    // Listener interface for section-specific events
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void mySectionValueChanged(float value) {}
    };

    MySidebarSection();
    ~MySidebarSection() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

    // State setters/getters
    void setMyValue(float value);
    float getMyValue() const { return myValue_; }

    void addListener(Listener* listener);
    void removeListener(Listener* listener);

    // Height calculation for layout
    int getPreferredHeight() const;

private:
    void setupComponents();
    void notifyValueChanged();

    // Section header
    std::unique_ptr<juce::Label> sectionLabel_;

    // Controls
    std::unique_ptr<juce::Slider> mySlider_;

    // State
    float myValue_ = 0.0f;

    juce::ListenerList<Listener> listeners_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MySidebarSection)
};

} // namespace oscil
```

**Step 2**: Create implementation at `src/ui/sections/MySidebarSection.cpp`

**Step 3**: Add to `CMakeLists.txt`:
```cmake
# In target_sources(Oscil PRIVATE ...)
src/ui/sections/MySidebarSection.cpp
```

**Step 4**: Add section to `SidebarComponent` and connect listener

## How to Create a Coordinator

Coordinators encapsulate state-UI coordination logic, reducing editor responsibilities.

**Step 1**: Create header at `include/ui/coordinators/MyCoordinator.h`

```cpp
/*
    Oscil - My Coordinator
    Handles [specific responsibility] for the plugin editor
*/

#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <functional>

namespace oscil
{

class MyCoordinator
{
public:
    using StateChangedCallback = std::function<void()>;

    MyCoordinator(StateChangedCallback onStateChanged);
    ~MyCoordinator();

    // Coordinator-specific methods
    void doSomething();
    int getValue() const { return value_; }

    // Prevent copying
    MyCoordinator(const MyCoordinator&) = delete;
    MyCoordinator& operator=(const MyCoordinator&) = delete;

private:
    void postToMessageThread(std::function<void()> callback);

    StateChangedCallback onStateChanged_;
    int value_ = 0;

    // Validity flag for async callbacks
    std::shared_ptr<std::atomic<bool>> isValid_ = std::make_shared<std::atomic<bool>>(true);
};

} // namespace oscil
```

**Step 2**: Create implementation at `src/ui/coordinators/MyCoordinator.cpp`

**Step 3**: Add to `CMakeLists.txt`:
```cmake
# In target_sources(Oscil PRIVATE ...)
src/ui/coordinators/MyCoordinator.cpp
```

**Step 4**: Instantiate in `PluginEditor` and connect to UI

## How to Create a Waveform Shader

OpenGL shaders render waveforms with GPU acceleration. Use the `WaveformShader` base class.

**Step 1**: Create header at `include/rendering/shaders/MyShader.h`

```cpp
/*
    Oscil - My Shader
    Description of the visual effect
*/

#pragma once

#include "rendering/WaveformShader.h"

namespace oscil
{

class MyShader : public WaveformShader
{
public:
    MyShader() = default;
    ~MyShader() override;

    // Shader identification
    [[nodiscard]] juce::String getId() const override { return "my_shader"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "My Shader"; }
    [[nodiscard]] juce::String getDescription() const override { return "Brief description"; }

#if OSCIL_ENABLE_OPENGL
    bool compile(juce::OpenGLContext& context) override;
    void release() override;
    [[nodiscard]] bool isCompiled() const override;
    void render(juce::OpenGLContext& context,
                const std::vector<float>& channel1,
                const std::vector<float>* channel2,
                const ShaderRenderParams& params) override;
#endif

    void renderSoftware(juce::Graphics& g,
                        const std::vector<float>& channel1,
                        const std::vector<float>* channel2,
                        const ShaderRenderParams& params) override;

private:
#if OSCIL_ENABLE_OPENGL
    std::unique_ptr<juce::OpenGLShaderProgram> shaderProgram_;
#endif
    bool compiled_ = false;
};

} // namespace oscil
```

**Step 2**: Create implementation at `src/rendering/shaders/MyShader.cpp`

**Step 3**: Register in `ShaderRegistry::registerBuiltInShaders()`:
```cpp
registerShader(std::make_unique<MyShader>());
```

**Step 4**: Add to `CMakeLists.txt`:
```cmake
src/rendering/shaders/MyShader.cpp
```

**Key Pattern**: Always provide `renderSoftware()` fallback for when OpenGL is disabled.

## How to Create a 3D Waveform Shader

3D shaders provide depth, lighting, and camera perspectives. Use the `WaveformShader3D` base class.

**Step 1**: Create header at `include/rendering/shaders3d/My3DShader.h`

```cpp
/*
    Oscil - My 3D Shader
    Description of the 3D visualization
*/

#pragma once

#include "rendering/shaders3d/WaveformShader3D.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

class My3DShader : public WaveformShader3D
{
public:
    My3DShader() = default;
    ~My3DShader() override;

    // Identification
    [[nodiscard]] juce::String getId() const override { return "my_3d_shader"; }
    [[nodiscard]] juce::String getName() const override { return "My 3D Shader"; }

    // Lifecycle
    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    // 3D Rendering
    void render(juce::OpenGLContext& context,
                const WaveformData3D& data,
                const Camera3D& camera,
                const LightingConfig& lighting) override;

private:
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    bool compiled_ = false;

    // Uniform locations
    GLint modelLoc_ = -1, viewLoc_ = -1, projLoc_ = -1;
    GLint colorLoc_ = -1, timeLoc_ = -1;
};

} // namespace oscil

#endif
```

**Step 2**: Create implementation at `src/rendering/shaders3d/My3DShader.cpp`

**Step 3**: Register in `ShaderRegistry::registerBuiltInShaders()`

**Step 4**: Add to `CMakeLists.txt`

**Key Pattern**: Use `setMatrixUniforms()` and `setLightingUniforms()` helpers from base class.

## How to Create a Post-Processing Effect

Post-processing effects apply screen-space transformations after waveform rendering.

**Step 1**: Create header at `include/rendering/effects/MyEffect.h`

```cpp
/*
    Oscil - My Effect
    Description of the screen effect
*/

#pragma once

#include "rendering/effects/PostProcessEffect.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

class MyEffect : public PostProcessEffect
{
public:
    MyEffect() = default;
    ~MyEffect() override = default;

    // Identification
    [[nodiscard]] juce::String getId() const override { return "my_effect"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "My Effect"; }

    // Lifecycle
    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    // Apply effect
    void apply(juce::OpenGLContext& context,
               Framebuffer* source,
               Framebuffer* destination,
               FramebufferPool& pool,
               float deltaTime) override;

private:
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    bool compiled_ = false;

    // Effect-specific uniforms
    GLint intensityLoc_ = -1;
    GLint textureLoc_ = -1;
};

} // namespace oscil

#endif
```

**Step 2**: Create implementation using `compileEffectShader()` helper

```cpp
bool MyEffect::compile(juce::OpenGLContext& context)
{
    shader_ = std::make_unique<juce::OpenGLShaderProgram>(context);

    // Fragment shader with vTexCoord from vertex shader
    const char* fragmentSource = R"(
        varying vec2 vTexCoord;
        uniform sampler2D uTexture;
        uniform float uIntensity;

        void main()
        {
            vec4 color = texture2D(uTexture, vTexCoord);
            // Apply your effect here
            gl_FragColor = color * uIntensity;
        }
    )";

    if (!compileEffectShader(*shader_, fragmentSource))
        return false;

    shader_->use();
    intensityLoc_ = shader_->getUniformIDFromName("uIntensity");
    textureLoc_ = shader_->getUniformIDFromName("uTexture");
    compiled_ = true;
    return true;
}
```

**Step 3**: Add to `RenderEngine` effect chain

**Step 4**: Add to `CMakeLists.txt`

**Key Pattern**: Use `compileEffectShader()` which provides the fullscreen quad vertex shader automatically.

## How to Create a Material Shader

Material shaders add physically-based rendering (PBR) properties like reflections and refraction.

**Step 1**: Create header at `include/rendering/materials/MyMaterial.h`

```cpp
/*
    Oscil - My Material
    PBR material description
*/

#pragma once

#include "rendering/materials/MaterialShader.h"

#if OSCIL_ENABLE_OPENGL

namespace oscil
{

class MyMaterial : public MaterialShader
{
public:
    MyMaterial() = default;
    ~MyMaterial() override = default;

    [[nodiscard]] juce::String getId() const override { return "my_material"; }
    [[nodiscard]] juce::String getName() const override { return "My Material"; }

    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    [[nodiscard]] bool isCompiled() const override;

    void render(juce::OpenGLContext& context,
                const WaveformData3D& data,
                const Camera3D& camera,
                const LightingConfig& lighting) override;

private:
    std::unique_ptr<juce::OpenGLShaderProgram> shader_;
    bool compiled_ = false;

    // Material uniform locations
    GLint metallicLoc_ = -1, roughnessLoc_ = -1, iorLoc_ = -1;
};

} // namespace oscil

#endif
```

**Step 2**: Create implementation using `setMaterialUniforms()` helper

**Step 3**: Add to `CMakeLists.txt`

**Key Pattern**: Extend `MaterialShader` (which extends `WaveformShader3D`) for PBR properties. Use `bindEnvironmentMap()` for reflections.

## How to Add a Theme Color

1. Add property to `ColorTheme` struct in `include/ui/ThemeManager.h`
2. Update `toValueTree()` and `fromValueTree()` in `src/ui/ThemeManager.cpp`
3. Use in components via `ThemeManager::getInstance().getCurrentTheme()`

## Thread Safety Rules

**Audio Thread** (in `processBlock()`):
- Use ONLY lock-free operations
- NO allocations after `prepareToPlay()`
- Use `SharedCaptureBuffer::write()` for data passing

**UI Thread**:
- Use `SharedCaptureBuffer::read()` for reading audio
- Use message queue for cross-thread communication
- All JUCE Component methods are UI-thread only

## Key Base Classes and When to Use Them

| Base Class | Use When |
|------------|----------|
| `juce::Component` | Creating any visual UI element |
| `juce::AudioProcessor` | Creating the main plugin (already done: `OscilPluginProcessor`) |
| `juce::OpenGLRenderer` | GPU-accelerated rendering (see `WaveformGLRenderer`) |
| `WaveformShader` | Creating custom 2D GPU waveform effects |
| `WaveformShader3D` | Creating 3D waveform visualizations with camera/lighting |
| `MaterialShader` | PBR material shaders (extends `WaveformShader3D`) |
| `PostProcessEffect` | Screen-space post-processing effects |
| `InstanceRegistryListener` | Reacting to source registration/removal |
| `ThemeManagerListener` | Reacting to theme changes |

## Singleton Pattern

Use sparingly. Current singletons:
- `InstanceRegistry::getInstance()` - Multi-instance coordination across plugin instances
- `ThemeManager::getInstance()` - Theme management and persistence
- `GlobalPreferences::getInstance()` - User preferences stored separately from project state (default theme, layout settings)
- `UIAudioFeedback::getInstance()` - Subtle audio feedback for UI interactions (clicks, toggles)

**Note**: `ShaderRegistry` is NOT a singleton. It uses a Factory pattern. Each `RenderEngine` instance owns its own `ShaderRegistry` to ensure OpenGL resources (compiled shaders) are isolated per-context.

## State Persistence

All persistent state uses `juce::ValueTree`. See `OscilState` for the pattern:
- Define schema as nested ValueTrees
- Use `toValueTree()` / `fromValueTree()` for serialization
- Store in `getStateInformation()` / `setStateInformation()`

## Common Mistakes

❌ **Don't**: Create raw pointers for audio buffers
✅ **Do**: Use `std::shared_ptr<SharedCaptureBuffer>`

❌ **Don't**: Allocate memory in `processBlock()`
✅ **Do**: Pre-allocate in `prepareToPlay()`

❌ **Don't**: Put headers in `src/` directory
✅ **Do**: Headers go in `include/`, implementations in `src/`

❌ **Don't**: Use underscores in file names for classes
✅ **Do**: Use `PascalCase.h` and `PascalCase.cpp`

❌ **Don't**: Forget the `oscil` namespace
✅ **Do**: Wrap all code in `namespace oscil { ... }`

❌ **Don't**: Use raw loops for audio in UI thread
✅ **Do**: Use `SignalProcessor::decimate()` to reduce samples for display

❌ **Don't**: Create new singletons unless absolutely necessary
✅ **Do**: Pass dependencies through constructors

❌ **Don't**: Forget to add new source files to `CMakeLists.txt`
✅ **Do**: Add every `.cpp` file to `target_sources(Oscil PRIVATE ...)`

❌ **Don't**: Lock mutexes in audio thread
✅ **Do**: Use lock-free atomics and ring buffers

❌ **Don't**: Forget `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClassName)` in Component classes
✅ **Do**: Add it as the last private member in every Component
