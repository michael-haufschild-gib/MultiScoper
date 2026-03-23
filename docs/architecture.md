# Architecture Guide for LLM Coding Agents

**Purpose**: Instructions for WHERE to put code and WHAT patterns to follow in this JUCE audio plugin.

**Tech Stack**: C++20, JUCE 8.0.12, OpenGL 3.3, CMake 3.25+, GoogleTest

---

## Core Architectural Principles

### 1. Layer-Based Architecture

**Mental Model**: Code flows downward through layers. Higher layers depend on lower layers, never the reverse.

```
plugin/     → Entry points (AudioProcessor, AudioProcessorEditor)
    ↓
ui/         → User interface (Components, Panels, Dialogs, Layout)
    ↓
rendering/  → OpenGL visualization (Shaders, Effects)
    ↓
core/       → Business logic (Oscillator, State, Registry, DSP)
```

**Rule**: Never include upward. Enforced by `scripts/architecture_lint.py`:
- `core/` must not include `ui/`, `rendering/`, `plugin/`, `tools/`
- `rendering/` must not include `ui/`, `plugin/`, `tools/`
- `ui/` must not include `plugin/`, `tools/`

### 2. Header/Source Separation

**Pattern**: Headers in `include/`, implementations in `src/`, mirrored structure.

```
include/core/Oscillator.h    ←→    src/core/Oscillator.cpp
include/ui/panels/Foo.h      ←→    src/ui/panels/Foo.cpp
```

**Rule**: Always use `#pragma once`. Include paths are relative to `include/`.

### 3. Dependency Injection

**Pattern**: Pass dependencies via constructor, not global singletons.

```cpp
// Good: Dependencies injected
OscilPluginProcessor(IInstanceRegistry& registry, IThemeService& theme);

// Bad: Hidden dependencies
OscilPluginProcessor() { registry_ = GlobalRegistry::getInstance(); }
```

---

## Where to Put New Code

### Decision Tree: "Where does X go?"

```
Is it an entry point (Processor/Editor)?     → plugin/
Is it a UI widget?                           → ui/components/
Is it a content panel (list, config)?        → ui/panels/
Is it a modal dialog?                        → ui/dialogs/
Is it a UI manager/coordinator?              → ui/managers/ or ui/controllers/
Is it layout/sidebar/pane management?        → ui/layout/
Is it a sidebar accordion section?           → ui/layout/sections/
Is it pane-internal (header, body, overlay)? → ui/layout/pane/
Is it theming/colors?                        → ui/theme/
Is it OpenGL rendering core?                 → rendering/
Is it a waveform shader?                     → rendering/shaders/
Is it a post-process effect?                 → rendering/effects/
Is it render subsystem logic?                → rendering/subsystems/
Is it business logic/models?                 → core/
Is it signal processing?                     → core/dsp/
Is it audio analysis?                        → core/analysis/
Is it an interface/abstraction?              → core/interfaces/
Is it test infrastructure?                   → tools/test_server/
Is it platform-specific (.mm)?               → platform/macos/
```

### Adding a New Feature Checklist

1. **Model** → `include/core/` + `src/core/`
2. **UI Component** → `include/ui/components/` + `src/ui/components/`
3. **Panel/Dialog** → `include/ui/panels/` or `ui/dialogs/`
4. **Tests** → `tests/test_*.cpp`
5. **Update Sources.cmake** → Add new files to `cmake/Sources.cmake`

---

## Class Patterns

### UI Component Pattern

All custom UI components follow this pattern:

```cpp
// include/ui/components/OscilFoo.h
#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include "ui/theme/ThemeManager.h"
#include "ui/components/ComponentConstants.h"
#include "ui/components/SpringAnimation.h"
#include "ui/components/TestId.h"

namespace oscil
{

class OscilFoo : public juce::Component,
                 public ThemeManagerListener,  // Theme changes
                 public TestIdSupport,         // E2E test IDs
                 private juce::Timer           // Animations
{
public:
    explicit OscilFoo(const juce::String& testId = {});
    ~OscilFoo() override;

    // Component overrides
    void paint(juce::Graphics& g) override;
    void resized() override;

    // ThemeManagerListener
    void themeChanged(const ColorTheme& newTheme) override;

private:
    void timerCallback() override;

    ColorTheme theme_;
    SpringAnimation hoverSpring_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OscilFoo)
};

} // namespace oscil
```

**Key Points**:
- Inherit `ThemeManagerListener` for theme support
- Inherit `TestIdSupport` for E2E testing
- Use `SpringAnimation` for smooth transitions
- Use `JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR`

### Model Pattern

Domain models use value semantics with unique IDs:

```cpp
// include/core/MyModel.h
#pragma once

#include <juce_core/juce_core.h>

namespace oscil
{

struct MyModelId
{
    juce::String id;

    bool operator==(const MyModelId& other) const { return id == other.id; }
    bool operator!=(const MyModelId& other) const { return !(*this == other); }

    [[nodiscard]] static MyModelId generate();
    static MyModelId invalid() { return MyModelId{ "" }; }
    bool isValid() const { return id.isNotEmpty(); }
};

class MyModel
{
public:
    MyModel();

    [[nodiscard]] const MyModelId& getId() const { return id_; }

    // Getters/setters...

private:
    MyModelId id_;
    // Properties...
};

} // namespace oscil
```

### Waveform Shader Pattern

All waveform shaders inherit from `WaveformShader`:

```cpp
// include/rendering/shaders/MyShader.h
#pragma once

#include "rendering/WaveformShader.h"

namespace oscil
{

class MyShader : public WaveformShader
{
public:
    MyShader() = default;
    ~MyShader() override = default;

    // Identification
    [[nodiscard]] juce::String getId() const override { return "my_shader"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "My Shader"; }
    [[nodiscard]] juce::String getDescription() const override { return "Description"; }

#if OSCIL_ENABLE_OPENGL
    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    bool isCompiled() const override;
    void render(juce::OpenGLContext& context,
                const std::vector<float>& samples,
                const ShaderRenderParams& params) override;
#endif

private:
    // OpenGL resources...
};

} // namespace oscil
```

### Post-Process Effect Pattern

Effects go in `rendering/effects/`:

```cpp
// include/rendering/effects/MyEffect.h
#pragma once

#include "rendering/effects/PostProcessEffect.h"

namespace oscil
{

class MyEffect : public PostProcessEffect
{
public:
    MyEffect() = default;
    ~MyEffect() override = default;

    [[nodiscard]] juce::String getId() const override { return "my_effect"; }
    [[nodiscard]] juce::String getDisplayName() const override { return "My Effect"; }

    void setIntensity(float intensity) { intensity_ = intensity; }

#if OSCIL_ENABLE_OPENGL
    bool compile(juce::OpenGLContext& context) override;
    void release(juce::OpenGLContext& context) override;
    void apply(juce::OpenGLContext& context,
               Framebuffer& input,
               Framebuffer& output,
               const EffectParams& params) override;
#endif

private:
    float intensity_ = 1.0f;
};

} // namespace oscil
```

---

## Interface Patterns

### When to Create an Interface

Create interfaces (`I*` prefix) when:
- Multiple implementations exist or will exist
- Unit testing requires mocking
- Breaking circular dependencies

**Location**: `include/core/interfaces/`

```cpp
// include/core/interfaces/IFoo.h
#pragma once

namespace oscil
{

class IFoo
{
public:
    virtual ~IFoo() = default;

    virtual void doSomething() = 0;
    virtual int getValue() const = 0;
};

} // namespace oscil
```

### Existing Interfaces

- `IInstanceRegistry` - Source registration abstraction
- `IAudioBuffer` - Audio buffer abstraction
- `IAudioDataProvider` - Audio data access
- `IThemeService` - Theme management

---

## Listener Pattern

JUCE uses listeners for callbacks. Follow this pattern:

```cpp
class MyComponent
{
public:
    class Listener
    {
    public:
        virtual ~Listener() = default;
        virtual void onSomethingHappened(const MyId& id) {}  // Default empty
        virtual void onValueChanged(float value) {}
    };

    void addListener(Listener* l) { listeners_.add(l); }
    void removeListener(Listener* l) { listeners_.remove(l); }

private:
    void notifySomethingHappened(const MyId& id)
    {
        listeners_.call([&](Listener& l) { l.onSomethingHappened(id); });
    }

    juce::ListenerList<Listener> listeners_;
};
```

---

## File Naming Conventions

| Type | Pattern | Example |
|------|---------|---------|
| UI Component | `Oscil{Name}.h/cpp` | `OscilButton.h` |
| Panel | `{Name}Component.h/cpp` | `SidebarComponent.h` |
| Dialog | `{Name}Dialog.h/cpp` | `AddOscillatorDialog.h` |
| Sidebar Section | `{Name}Section.h/cpp` | `TimingSidebarSection.h` |
| Model | `{Name}.h/cpp` | `Oscillator.h` |
| Shader | `{Name}Shader.h/cpp` | `NeonGlowShader.h` |
| Effect | `{Name}Effect.h/cpp` | `BloomEffect.h` |
| Test | `test_{name}.cpp` | `test_oscillator.cpp` |
| Interface | `I{Name}.h` | `IInstanceRegistry.h` |
| Manager | `{Name}Manager.h/cpp` | `DialogManager.h` |
| Controller | `{Name}Controller.h/cpp` | `OscillatorPanelController.h` |
| Coordinator | `{Name}Coordinator.h/cpp` | `LayoutCoordinator.h` |

---

## Include Path Patterns

```cpp
// Core includes
#include "core/Oscillator.h"
#include "core/OscilState.h"
#include "core/interfaces/IInstanceRegistry.h"
#include "core/dsp/TimingEngine.h"
#include "core/analysis/AnalysisEngine.h"

// UI includes
#include "ui/components/OscilButton.h"
#include "ui/panels/WaveformComponent.h"
#include "ui/dialogs/AddOscillatorDialog.h"
#include "ui/theme/ThemeManager.h"
#include "ui/layout/SidebarComponent.h"
#include "ui/layout/sections/TimingSidebarSection.h"
#include "ui/managers/DialogManager.h"
#include "ui/controllers/OscillatorPanelController.h"

// Rendering includes
#include "rendering/WaveformShader.h"
#include "rendering/shaders/NeonGlowShader.h"
#include "rendering/effects/BloomEffect.h"
#include "rendering/subsystems/EffectPipeline.h"

// Plugin includes (only from plugin/ or ui/)
#include "plugin/PluginProcessor.h"
#include "plugin/PluginEditor.h"
```

---

## Adding New Files to Build

After creating new `.cpp` files, add them to `cmake/Sources.cmake`:

```cmake
# In cmake/Sources.cmake, find the appropriate section:

set(OSCIL_SOURCES
    # ... existing sources ...

    # Add your new file in the appropriate section:
    ${CMAKE_SOURCE_DIR}/src/ui/components/OscilNewWidget.cpp
)
```

Then rebuild:
```bash
cmake --build --preset dev
```

---

## Thread Domains

### Audio Thread (Real-Time Safe)
- `PluginProcessor::processBlock()`
- `TimingEngine` timing calculations
- `SharedCaptureBuffer` writes

**Rules**: No allocations, no locks, no I/O, no exceptions.

### Message Thread (UI)
- All `juce::Component` methods
- State modifications
- UI updates

### OpenGL Thread
- `WaveformGLRenderer::renderOpenGL()`
- All shader rendering
- Effect pipeline

**Rule**: Use `SpinLock` for data shared between UI and OpenGL threads.

---

## Common Mistakes

**Don't**: Include plugin headers from core/
```cpp
// Bad: core/ depending on plugin/
#include "plugin/PluginProcessor.h"  // NO!
```
**Do**: Use interfaces and dependency injection

---

**Don't**: Create global singletons in new code
```cpp
// Bad: Global state
auto& registry = InstanceRegistry::getInstance();
```
**Do**: Inject dependencies via constructor

---

**Don't**: Put headers in src/
```cpp
// Bad: Header in implementation folder
src/ui/MyHelper.h
```
**Do**: Headers in include/, implementations in src/

---

**Don't**: Forget to update Sources.cmake
```cmake
# After adding new .cpp files, update cmake/Sources.cmake
```
**Do**: Add both .h and .cpp paths, then rebuild

---

**Don't**: Allocate in audio thread
```cpp
// Bad: Allocation in processBlock
void processBlock(...) {
    auto* buffer = new float[1024];  // NO!
}
```
**Do**: Pre-allocate in constructor, use lock-free structures

---

---

## Quick Reference

### Key Base Classes
- `juce::Component` - All UI components
- `juce::AudioProcessor` - Plugin processor
- `juce::AudioProcessorEditor` - Plugin editor
- `ThemeManagerListener` - Theme-aware components
- `TestIdSupport` - E2E testable components
- `WaveformShader` - Waveform shaders
- `PostProcessEffect` - Post-processing effects

### Key Namespaces
- `oscil` - All project code
- `juce` - JUCE framework (use explicitly)

### Build Commands
```bash
cmake --preset dev          # Configure
cmake --build --preset dev  # Build
ctest --preset dev          # Test
```
