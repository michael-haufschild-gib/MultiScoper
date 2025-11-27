# Architecture Guide for LLM Coding Agents

**Purpose**: Instructions for where to put code and what patterns to follow in the Oscil codebase.
**Tech Stack**: C++20, JUCE 8.0.4, CMake, GoogleTest, cpp-httplib, nlohmann/json

## Project Overview

Oscil is a multi-track audio oscilloscope VST/AU plugin. All code lives in the `oscil` namespace.

## Where to Put New Code

```
include/                    # Header files (.h)
├── core/                   # Core logic: processors, state, data models
├── dsp/                    # Digital signal processing algorithms
├── ui/                     # UI components (juce::Component subclasses)
│   ├── sections/           # Sidebar collapsible sections
│   └── coordinators/       # State-UI coordination logic
└── Oscil.h                 # Main umbrella header

src/                        # Implementation files (.cpp)
├── core/                   # Core implementations
├── dsp/                    # DSP implementations
└── ui/                     # UI implementations
    ├── sections/           # Sidebar section implementations
    └── coordinators/       # Coordinator implementations

tests/                      # GoogleTest unit tests
└── test_*.cpp              # Test files

test_harness/               # E2E test harness (separate JUCE app)
├── include/                # Test harness headers
└── src/                    # Test harness implementations
```

**Decision tree - where does my code go?**
- Audio processing logic? → `src/dsp/` + `include/dsp/`
- State management/data models? → `src/core/` + `include/core/`
- UI rendering/components? → `src/ui/` + `include/ui/`
- Collapsible sidebar section? → `src/ui/sections/` + `include/ui/sections/`
- State-UI coordination? → `src/ui/coordinators/` + `include/ui/coordinators/`
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
| `InstanceRegistryListener` | Reacting to source registration/removal |
| `ThemeManagerListener` | Reacting to theme changes |

## Singleton Pattern

Use sparingly. Current singletons:
- `InstanceRegistry::getInstance()` - Multi-instance coordination across plugin instances
- `ThemeManager::getInstance()` - Theme management and persistence
- `GlobalPreferences::getInstance()` - User preferences stored separately from project state (default theme, layout settings)

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
